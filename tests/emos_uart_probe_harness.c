#include <assert.h>
#include <string.h>
#include "uart.h"
#include "emos_uart_probe.h"

volatile BYTE serialFlags;
BYTE uart1_claim_rts(void) { assert(0); return UART_POLL_UNAVAILABLE; }
BYTE uart1_receive_ready(BYTE ready) { (void)ready; assert(0); return UART_POLL_UNAVAILABLE; }
static const char expected[] = "EMOS UART1 -> P4\r\n";
static const char *response;
static unsigned sent, reads, replies, opened, closed, ticks;
static int scenario;

BYTE emos_uart_probe_clock(void) {
    if (scenario != 7 && !(scenario == 8 && sent == sizeof(expected) - 1)) ticks += 2;
    return (BYTE)ticks;
}

BYTE open_UART1(UART *settings) {
    ++opened;
    assert(settings->baudRate == 115200 && settings->dataBits == 8);
    assert(settings->stopBits == 1 && settings->parity == 0);
    assert(settings->flowControl == 0 && settings->interrupts == 0);
    if (scenario == 9) return UART_ERR_FAILURE;
    serialFlags |= 0x10;
    return 0;
}

void close_UART1(void) { ++closed; serialFlags &= 0x0F; }

BYTE uart1_try_put(BYTE value) {
    if (scenario == 6) return UART_POLL_EMPTY;
    if (scenario == 10) return UART_POLL_UNAVAILABLE;
    assert(sent < sizeof(expected) - 1 && value == (BYTE)expected[sent]);
    ++sent;
    return UART_POLL_READY;
}

BYTE uart1_try_get(BYTE *value) {
    ++reads;
    if (scenario == 5) return UART_POLL_ERROR;
    if (scenario == 11) return UART_POLL_UNAVAILABLE;
    if (scenario == 12 && replies == 5) return UART_POLL_ERROR;
    /* Delayed, fragmented receipt exercises quiet timing and full timeout. */
    if (reads > 6 && reads % 3 == 0 && response[replies]) {
        *value = (BYTE)response[replies++];
        return UART_POLL_READY;
    }
    return UART_POLL_EMPTY;
}

int main(void) {
    for (scenario = 0; scenario <= 13; ++scenario) {
        int passed;
        serialFlags = 0;
        sent = reads = replies = opened = closed = 0;
        ticks = scenario == 13 ? 250 : 0;
        response = scenario == 1 ? "" : scenario == 2 ? "AC" :
                   scenario == 3 ? "AXK\r\n" : scenario == 4 ? "ACK\r\nX" : "ACK\r\n";
        passed = emos_uart_probe();
        assert(passed == (scenario == 0 || scenario == 13));
        assert(opened == 1 && closed == (scenario == 9 ? 0 : 1));
        assert((serialFlags & 0x10) == 0);
        if (scenario == 0 || scenario == 13) assert(sent == 18 && replies == 5);
    }
    serialFlags = 0x10;
    opened = closed = 0;
    assert(!emos_uart_probe());
    assert(opened == 0 && closed == 0 && serialFlags == 0x10);
    return 0;
}
