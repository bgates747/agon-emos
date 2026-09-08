#include <assert.h>
#include <stdio.h>
#include "uart.h"
#include "emos_uart_flow.h"
volatile BYTE serialFlags;
static unsigned mode, phase, ticks, phase_at, sent, received, closes;
static unsigned owns_rts, ready_calls;
BYTE emos_uart_flow_clock(void) { if (mode != 9) ticks += 2; return (BYTE)ticks; }
BYTE open_UART1(UART *s) {
    assert(s->baudRate == 1152000 && s->flowControl == FCTL_HW && !s->interrupts);
    assert(!(serialFlags & 0x10)); serialFlags |= 0x30; return UART_ERR_NONE;
}
void close_UART1(void) { ++closes; serialFlags &= 0x0F; owns_rts = 0; }
BYTE uart1_claim_rts(void) { if (mode == 11) return UART_POLL_UNAVAILABLE; owns_rts = 1; return UART_POLL_READY; }
BYTE uart1_receive_ready(BYTE ready) {
    assert(owns_rts);
    if (mode == 12 && ready_calls == 2) return UART_POLL_UNAVAILABLE;
    ++ready_calls; ++phase; phase_at = ticks;
    assert(ready == (phase & 1)); return UART_POLL_READY;
}
BYTE uart1_try_put(BYTE value) {
    static const char request[] = "FLOW\r\n";
    if (phase == 1) {
        if (mode == 1 || (mode != 2 && ticks - phase_at < 120)) return UART_POLL_BLOCKED;
        if (mode == 3) return UART_POLL_ERROR;
        assert(sent < sizeof(request)-1 && value == (BYTE)request[sent]);
        ++sent; return UART_POLL_READY;
    }
    assert(phase == 3 && value == 0xA5);
    return mode == 8 ? UART_POLL_READY : UART_POLL_BLOCKED;
}
BYTE uart1_try_get(BYTE *value) {
    static const char ack[] = "FLOWACK\r\n";
    if ((phase == 2 && mode == 4) || (phase == 4 && mode == 13)) {
        *value = '!'; return UART_POLL_READY;
    }
    if (mode == 14) return UART_POLL_ERROR;
    if (phase == 3) {
        if (mode == 6 && received == 3) return UART_POLL_EMPTY;
        if (received < sizeof(ack)-1) {
            *value = mode == 5 ? 'X' : (BYTE)ack[received++]; return UART_POLL_READY;
        }
        if (mode == 7) { *value = '!'; return UART_POLL_READY; }
    }
    return UART_POLL_EMPTY;
}
int main(void) {
    for (mode=0; mode<15; ++mode) {
        phase=0; ticks=250; phase_at=250; sent=received=closes=ready_calls=owns_rts=0;
        serialFlags= mode == 10 ? 0x10 : 0;
        int pass=emos_uart_flow();
        assert(pass == (mode == 0));
        assert(closes == (mode == 10 ? 0 : 1));
        assert(!owns_rts);
        assert(serialFlags == (mode == 10 ? 0x10 : 0));
    }
    puts("15 UART flow command scenarios passed");
    return 0;
}
