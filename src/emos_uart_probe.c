/* INTEG-004: Core-owned, one-shot UART diagnostic. Only EMOS invokes this
 * after checking Legacy mode. Standard MOS blocking APIs remain unchanged.
 * All waits below poll nonblocking EMOS driver functions and have deadlines.
 * Use the maintained MOS printf path: AgonDev puts/putchar are deliberately
 * excluded from the restricted runtime because they bypass MOS output.
 */
#include <stdio.h>
#include <defines.h>
#include "uart.h"
#include "emos_uart_probe.h"

static const char request[] = "EMOS UART1 -> P4\r\n";
static const char reply[] = "ACK\r\n";

/* MOS increments the low clock byte by two per VBlank. At 60 Hz, 600 units
 * allow about five seconds. This is not a millisecond clock. Sampling one
 * byte avoids torn reads; frequent polling accumulates across byte wraps.
 * The separate poll budget terminates even if the VBlank clock stops.
 */
#define TX_CLOCK_LIMIT 120
#define RX_CLOCK_LIMIT 600
#define QUIET_CLOCK_UNITS 24
#define STALLED_POLL_LIMIT 65535UL

#ifdef EMOS_UART_PROBE_HOST_TEST
/* Tests replace only the clock source; the command and wait logic are real. */
extern BYTE emos_uart_probe_clock(void);
#else
extern volatile BYTE sysvars[];
static BYTE emos_uart_probe_clock(void) { return sysvars[0]; }
#endif

typedef struct {
    BYTE last;
    UINT16 elapsed;
    UINT32 stalled;
} ProbeClock;

static void clock_start(ProbeClock *clock) {
    clock->last = emos_uart_probe_clock();
    clock->elapsed = 0;
    clock->stalled = STALLED_POLL_LIMIT;
}

static BYTE clock_step(ProbeClock *clock) {
    BYTE now = emos_uart_probe_clock();
    BYTE delta = (BYTE)(now - clock->last);
    clock->last = now;
    clock->elapsed += delta;
    if (delta) clock->stalled = STALLED_POLL_LIMIT;
    else if (--clock->stalled == 0) return 0;
    return 1;
}

int emos_uart_probe(void) {
    UART settings = {115200, 8, 1, 0, 0, 0};
    ProbeClock clock;
    BYTE sent = 0, received = 0, value = 0, status;
    UINT16 complete_at = 0;
    const char *failure = NULL;

    /* Never replace somebody else's open UART or close it on rejection. */
    if (serialFlags & 0x10) {
        printf("UART ROUND TRIP FAIL: UART1 is already in use\r\n");
        return 0;
    }
    if (open_UART1(&settings) != UART_ERR_NONE) {
        printf("UART ROUND TRIP FAIL: EMOS could not open UART1\r\n");
        return 0;
    }
    printf("UART ROUND TRIP: waiting for acknowledgement...\r\n");
    clock_start(&clock);
    while (sent < sizeof(request) - 1) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; break; }
        if (clock.elapsed >= TX_CLOCK_LIMIT) { failure = "transmit timeout"; break; }
        status = uart1_try_put((BYTE)request[sent]);
        if (status == UART_POLL_READY) ++sent;
        else if (status != UART_POLL_EMPTY) { failure = "UART transmit unavailable"; break; }
    }
    if (!failure) {
        clock_start(&clock);
        for (;;) {
            if (!clock_step(&clock)) { failure = "MOS clock stalled"; break; }
            if (clock.elapsed >= RX_CLOCK_LIMIT) {
                failure = received ? "incomplete reply or quiet interval" : "no reply from P4";
                break;
            }
            status = uart1_try_get(&value);
            if (status == UART_POLL_READY) {
                if (received == sizeof(reply) - 1) { failure = "extra reply bytes"; break; }
                if (value != (BYTE)reply[received]) { failure = "wrong reply bytes"; break; }
                ++received;
                if (received == sizeof(reply) - 1) complete_at = clock.elapsed;
            } else if (status != UART_POLL_EMPTY) {
                failure = "UART receive error or unavailable";
                break;
            }
            if (received == sizeof(reply) - 1 &&
                clock.elapsed - complete_at >= QUIET_CLOCK_UNITS) break;
        }
    }
    close_UART1();
    if (failure) {
        printf("UART ROUND TRIP FAIL: %s\r\n", failure);
        return 0;
    }
    printf("UART ROUND TRIP PASS\r\n");
    return 1;
}
