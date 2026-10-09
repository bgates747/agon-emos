/* PORT-008 UARTFLOW extraction: original INTEG-006 algorithm, now on SD.
 * Only flow_* service adapters access resident transport. MOSlet owns waits,
 * expected bytes and messages; no direct UART/GPIO or installed IRQ callback.
 */
#include <stdio.h>
#include "flow.h"
static const char request[] = "FLOW\r\n";
static const char reply[] = "FLOWACK\r\n";

/* A 60 Hz MOS clock advances by two per VBlank, not one per millisecond. */
#define SECOND 120
#define STALLED_BUDGET 65535UL
typedef struct { BYTE last; UINT16 elapsed; UINT32 stalled; } FlowClock;
static FlowClock timer;
static const char *failure;
static void start(void) {
    timer.last = emos_uart_flow_clock(); timer.elapsed = 0; timer.stalled = STALLED_BUDGET;
}
static BYTE step(void) {
    BYTE now = emos_uart_flow_clock(), delta = (BYTE)(now - timer.last);
    timer.last = now; timer.elapsed += delta;
    if (delta) timer.stalled = STALLED_BUDGET;
    else if (--timer.stalled == 0) { failure = "MOS clock stalled"; return 0; }
    return 1;
}
static BYTE ready(BYTE value) {
    if (flow_ready(value) == UART_POLL_READY) return 1;
    failure = "RTS ownership lost"; return 0;
}
static BYTE silence(UINT16 units) {
    BYTE value, status;
    start();
    while (step()) {
        status = flow_get(&value);
        if (status != UART_POLL_EMPTY) {
            failure = status == UART_POLL_READY ? "unexpected bytes during hold/quiet" : "UART receive error";
            return 0;
        }
        if (timer.elapsed >= units) return 1;
    }
    return 0;
}
int emos_uart_flow(void) {
    BYTE sent = 0, received = 0, value, status;
    UINT16 blocked_until = 0;
    failure = NULL;
    if (!flow_open()) {
        printf("UART FLOW FAIL: diagnostic service unavailable or UART1 busy\r\n"); return 0;
    }
    printf("UART FLOW: 1152000 baud; testing pause, resume and blocked timeouts...\r\n");
    if (!ready(1)) goto done; /* Start edge observed by P4 CTS; P4 still stops TX. */
    start();
    while (sent < sizeof(request) - 1 && step()) {
        if (timer.elapsed >= 3 * SECOND) { failure = "CTS did not release"; break; }
        status = flow_put((BYTE)request[sent]);
        if (status == UART_POLL_BLOCKED) {
            if (sent) { failure = "CTS stopped inside request"; break; }
            blocked_until = timer.elapsed;
        } else if (status == UART_POLL_READY) {
            if (blocked_until < SECOND / 2) { failure = "peer did not hold CTS"; break; }
            ++sent;
        } else if (status != UART_POLL_EMPTY) { failure = "UART transmit error"; break; }
    }
    if (failure) goto done;
    printf("UART FLOW FORWARD PAUSE PASS\r\n");
    /* P4 queues ACK only after complete request and this stop edge. */
    if (!ready(0) || !silence(SECOND) || !ready(1)) goto done;
    start();
    while (received < sizeof(reply) - 1 && step()) {
        if (timer.elapsed >= 3 * SECOND) { failure = "missing or incomplete reply"; break; }
        status = flow_get(&value);
        if (status == UART_POLL_READY) {
            if (value != (BYTE)reply[received]) { failure = "wrong reply bytes"; break; }
            ++received;
        } else if (status != UART_POLL_EMPTY) { failure = "UART receive error"; break; }
    }
    if (failure || !silence(SECOND / 5)) goto done;
    printf("UART FLOW RETURN PAUSE PASS\r\n");
    /* P4 keeps its RTS HIGH after the request. No byte may enter our TX FIFO. */
    start();
    while (step()) {
        status = flow_put(0xA5);
        if (status != UART_POLL_BLOCKED) { failure = "blocked transmit was not stopped by CTS"; break; }
        status = flow_get(&value);
        if (status != UART_POLL_EMPTY) { failure = "unexpected receive during blocked transmit"; break; }
        if (timer.elapsed >= SECOND) break;
    }
    if (failure) goto done;
    printf("UART FLOW BLOCKED TX PASS\r\n");
    /* Final stop edge asks P4 to queue one byte and prove its own TX timeout.
     * P4 capture must attest that attempt, timeout and cancellation separately.
     */
    if (!ready(0) || !silence(3 * SECOND)) goto done;
    printf("UART FLOW BLOCKED RETURN QUIET PASS\r\n");
done:
    flow_close(); /* Releases the RTS output even on an error or stopped clock. */
    if (failure) { printf("UART FLOW FAIL: %s\r\n", failure); return 0; }
    printf("UART FLOW PASS - returning to MOS\r\n");
    return 1;
}
