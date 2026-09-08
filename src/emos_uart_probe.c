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

/* INTEG-007: MOS/VDP General Poll belongs to EMOS, never an application.
 * Stock UART0's GP/sysvars are left alone: this bounded UART1 transaction
 * validates its own reply before returning to the ordinary Legacy console.
 */
int emos_general_poll(void) {
    static const BYTE poll[] = {23, 0, 0x80, 0xA5};
    static const BYTE expected[] = {0x80, 1, 0xA5};
    UART settings = {1152000, 8, 1, 0, FCTL_HW, 0};
    ProbeClock clock;
    BYTE sent = 0, received = 0, value, status;
    UINT16 complete_at = 0;
    const char *failure = NULL;
    if (serialFlags & 0x10) {
        printf("VDP POLL FAIL: UART1 is already in use\r\n");
        return 0;
    }
    if (open_UART1(&settings) != UART_ERR_NONE) {
        printf("VDP POLL FAIL: EMOS could not open UART1\r\n");
        return 0;
    }
    if (uart1_claim_rts() != UART_POLL_READY) {
        failure = "PC2 RTS is unavailable"; goto done;
    }
    printf("VDP POLL: 1152000 baud; waiting for EDP General Poll...\r\n");
    if (uart1_receive_ready(1) != UART_POLL_READY) {
        failure = "RTS unavailable"; goto done;
    }
    clock_start(&clock);
    while (sent < sizeof(poll)) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; goto done; }
        if (clock.elapsed >= TX_CLOCK_LIMIT) { failure = "transmit timeout"; goto done; }
        status = uart1_try_put(poll[sent]);
        if (status == UART_POLL_READY) ++sent;
        else if (status != UART_POLL_EMPTY && status != UART_POLL_BLOCKED) { failure = "UART transmit unavailable"; goto done; }
    }
    clock_start(&clock);
    for (;;) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; break; }
        if (clock.elapsed >= RX_CLOCK_LIMIT) {
            failure = received ? "incomplete reply or quiet interval" : "no reply from EDP";
            break;
        }
        status = uart1_try_get(&value);
        if (status == UART_POLL_READY) {
            if (received == sizeof(expected)) { failure = "extra reply bytes"; break; }
            if (value != expected[received]) { failure = "wrong General Poll reply"; break; }
            if (++received == sizeof(expected)) complete_at = clock.elapsed;
        } else if (status != UART_POLL_EMPTY) { failure = "UART receive error"; break; }
        if (received == sizeof(expected) && clock.elapsed - complete_at >= QUIET_CLOCK_UNITS) break;
    }
 done:
    close_UART1();
    if (failure) { printf("VDP POLL FAIL: %s\r\n", failure); return 0; }
    printf("VDP POLL PASS: reply 80 01 A5 - returning to MOS\r\n");
    return 1;
}

/* INTEG-008: bounded qualification text, separate from ordinary VDU routing.
 * Applications supply text only. Core adds flush/General Poll, owns UART1,
 * validates the actual reply and closes/restores the port on every exit.
 * The original VDPTEXT command retains its A6 exchange for older procedures.
 */
int emos_text_valid(const BYTE *text, UINT16 length) {
    UINT16 i;
    if (!text || !length || length > EMOS_TEXT_LIMIT) return 0;
    for (i = 0; i < length; ++i) {
        BYTE b = text[i];
        if (b >= 32 && b <= 126) continue;
        if ((b >= 8 && b <= 13) || b == 30) continue;
        if (b == 31 && length - i >= 3) { i += 2; continue; }
        return 0;
    }
    return 1;
}

static int text_exchange(const BYTE *text, UINT16 length, BYTE token, BYTE verbose) {
    BYTE suffix[] = {23, 0, 0xCA, 23, 0, 0x80, 0};
    BYTE expected[] = {0x80, 1, 0};
    UART settings = {1152000, 8, 1, 0, FCTL_HW, 0};
    ProbeClock clock;
    UINT16 sent = 0;
    BYTE received = 0, value, status;
    UINT16 complete_at = 0;
    const char *failure = NULL;
    suffix[6] = expected[2] = token;
    if (serialFlags & 0x10) {
        printf("VDP TEXT FAIL: UART1 is already in use\r\n");
        return 0;
    }
    if (open_UART1(&settings) != UART_ERR_NONE) {
        printf("VDP TEXT FAIL: EMOS could not open UART1\r\n");
        return 0;
    }
    if (uart1_claim_rts() != UART_POLL_READY) {
        failure = "PC2 RTS is unavailable"; goto done;
    }
    if (verbose) printf("VDP TEXT: 1152000 baud; sending text for the browser...\r\n");
    if (uart1_receive_ready(1) != UART_POLL_READY) {
        failure = "RTS unavailable"; goto done;
    }
    clock_start(&clock);
    while (sent < length + sizeof(suffix)) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; goto done; }
        if (clock.elapsed >= TX_CLOCK_LIMIT) { failure = "transmit timeout"; goto done; }
        status = uart1_try_put(sent < length ? text[sent] : suffix[sent - length]);
        if (status == UART_POLL_READY) ++sent;
        else if (status != UART_POLL_EMPTY && status != UART_POLL_BLOCKED) { failure = "UART transmit unavailable"; goto done; }
    }
    clock_start(&clock);
    for (;;) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; break; }
        if (clock.elapsed >= RX_CLOCK_LIMIT) {
            failure = received ? "incomplete reply or quiet interval" : "no reply from EDP";
            break;
        }
        status = uart1_try_get(&value);
        if (status == UART_POLL_READY) {
            if (received == sizeof(expected)) { failure = "extra reply bytes"; break; }
            if (value != expected[received]) { failure = "wrong text completion reply"; break; }
            if (++received == sizeof(expected)) complete_at = clock.elapsed;
        } else if (status != UART_POLL_EMPTY) { failure = "UART receive error"; break; }
        if (received == sizeof(expected) && clock.elapsed - complete_at >= QUIET_CLOCK_UNITS) break;
    }
 done:
    close_UART1();
    if (failure) { printf("VDP TEXT FAIL: %s\r\n", failure); return 0; }
    if (verbose) printf("VDP TEXT PASS: parser ACK - confirm browser text; returning to MOS\r\n");
    return 1;
}

int emos_text_probe(const BYTE *text, UINT16 length) {
    if (!emos_text_valid(text, length)) return 0;
    return text_exchange(text, length, 0xA7, 0);
}

int emos_visible_text(void) {
    static const BYTE text[] = "\x0c\x1f\x02\x02" "EMOS TO EDP: UART TEXT\r\n";
    return text_exchange(text, sizeof(text) - 1, 0xA6, 1);
}
