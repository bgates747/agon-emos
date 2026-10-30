/* INTEG-017: external finite text diagnostic. This MOSlet owns text grammar,
 * sequence, clock policy and reporting. EMOS ext.uartdiag owns UART1, RTS and
 * abandoned-lease cleanup; no application register access or ISR callback.
 * The A6 fixed exchange and A7 caller-text variant retain their original bytes.
 * Gateway call overhead changes polling cadence; nominal units are not ms. */
#include <stdio.h>
#include "probe.h"
#define emos_uart_probe_clock emos_uart_flow_clock
#include "../../../src/emos_probe_clock.h"
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
    ProbeClock clock;
    UINT16 sent = 0;
    BYTE received = 0, value, status;
    UINT16 complete_at = 0;
    const char *failure = NULL;
    suffix[6] = expected[2] = token;
    if (!flow_open()) {
        printf("VDP TEXT FAIL: diagnostic service unavailable or UART1 busy\r\n");
        return 0;
    }
    if (verbose) printf("VDP TEXT: 1152000 baud; sending text for the browser...\r\n");
    if (flow_ready(1) != UART_POLL_READY) {
        failure = "RTS unavailable"; goto done;
    }
    clock_start(&clock);
    while (sent < length + sizeof(suffix)) {
        if (!clock_step(&clock)) { failure = "MOS clock stalled"; goto done; }
        if (clock.elapsed >= TX_CLOCK_LIMIT) { failure = "transmit timeout"; goto done; }
        status = flow_put(sent < length ? text[sent] : suffix[sent - length]);
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
            if (value != expected[received]) { failure = "wrong text completion reply"; break; }
            if (++received == sizeof(expected)) complete_at = clock.elapsed;
        } else if (status != UART_POLL_EMPTY) { failure = "UART receive error"; break; }
        if (received == sizeof(expected) && clock.elapsed - complete_at >= QUIET_CLOCK_UNITS) break;
    }
 done:
    flow_close();
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
