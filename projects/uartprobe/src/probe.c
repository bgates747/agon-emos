/* PORT-008: extracted INTEG-004/007 diagnostics; preserve wire/deadlines.
 * Reuse UARTFLOW's API 0x51 adapter. No raw UART/GPIO, vectors or route access.
 * Lease refusals now report service unavailable/busy; only EMOS can distinguish
 * and clean up an acquisition failure. The successful sequences are unchanged.
 */
#include <stdio.h>
#include "probe.h"
#define emos_uart_probe_clock emos_uart_flow_clock
#include "../../../src/emos_probe_clock.h"
static const char request[] = "EMOS UART1 -> P4\r\n";
static const char reply[] = "ACK\r\n";

int emos_uart_probe(void) {
    ProbeClock clock;
    BYTE sent = 0, received = 0, value = 0, status;
    UINT16 complete_at = 0;
    const char *failure = NULL;

    /* EMOS admits the utility and refuses other UART/pin owners. */
    if (!flow_open_slow()) {
        printf("UART ROUND TRIP FAIL: diagnostic service unavailable or UART1 busy\r\n");
        return 0;
    }
    printf("UART ROUND TRIP: waiting for acknowledgement...\r\n");
    clock_start(&clock);
    while (sent < sizeof(request) - 1) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; break; }
        if (clock.elapsed >= TX_CLOCK_LIMIT) { failure = "transmit timeout"; break; }
        status = flow_put((BYTE)request[sent]);
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
            status = flow_get(&value);
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
    flow_close();
    if (failure) {
        printf("UART ROUND TRIP FAIL: %s\r\n", failure);
        return 0;
    }
    printf("UART ROUND TRIP PASS\r\n");
    return 1;
}

/* Original INTEG-007 General Poll sequence. EMOS owns UART1 through the
 * admitted gateway; this MOSlet leaves stock UART0 GP/sysvars untouched. */
int emos_general_poll(void) {
    static const BYTE poll[] = {23, 0, 0x80, 0xA5};
    static const BYTE expected[] = {0x80, 1, 0xA5};
    ProbeClock clock;
    BYTE sent = 0, received = 0, value, status;
    UINT16 complete_at = 0;
    const char *failure = NULL;
    /* Fast acquisition includes EMOS-owned RTS claim and failure cleanup. */
    if (!flow_open()) {
        printf("VDP POLL FAIL: diagnostic service unavailable or UART1 busy\r\n");
        return 0;
    }
    printf("VDP POLL: 1152000 baud; waiting for EDP General Poll...\r\n");
    if (flow_ready(1) != UART_POLL_READY) {
        failure = "RTS unavailable"; goto done;
    }
    clock_start(&clock);
    while (sent < sizeof(poll)) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; goto done; }
        if (clock.elapsed >= TX_CLOCK_LIMIT) { failure = "transmit timeout"; goto done; }
        status = flow_put(poll[sent]);
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
        status = flow_get(&value);
        if (status == UART_POLL_READY) {
            if (received == sizeof(expected)) { failure = "extra reply bytes"; break; }
            if (value != expected[received]) { failure = "wrong General Poll reply"; break; }
            if (++received == sizeof(expected)) complete_at = clock.elapsed;
        } else if (status != UART_POLL_EMPTY) { failure = "UART receive error"; break; }
        if (received == sizeof(expected) && clock.elapsed - complete_at >= QUIET_CLOCK_UNITS) break;
    }
 done:
    flow_close();
    if (failure) { printf("VDP POLL FAIL: %s\r\n", failure); return 0; }
    printf("VDP POLL PASS: reply 80 01 A5 - returning to MOS\r\n");
    return 1;
}
