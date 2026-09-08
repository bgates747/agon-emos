/* INTEG-003 diagnostic caller of the INSTALLED EMOS UART1 driver.
 * No direct hardware access or product routing/activation implementation.
 * Run only through the controlled Legacy EMOS STATUS / LOAD / RUN autoexec.
 * The peer does not drive any harness pin; CTS and receive interrupts are off.
 */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include "uart_forward_probe_config.h"

_Static_assert(sizeof(UART) == 8, "MOS UART settings ABI changed");

static int settle_transmitter(void) {
    const volatile uint8_t *clock = mos_sysvars();
    uint8_t last = clock[sysvar_time];
    uint8_t transitions = 0;
    uint24_t budget = 0xFFFFFF;
    /* PUTCH means queued, not physically drained. MOS has no public drain
     * call. Eight observed clock changes provide ample settling for this
     * short 115200-baud frame. The budget prevents a stalled-clock hang;
     * it is not a calibrated timeout. Do not substitute direct UART polling.
     */
    while (transitions < 8 && --budget) {
        const uint8_t now = clock[sysvar_time];
        if (now != last) {
            ++transitions;
            last = now;
        }
    }
    return transitions == 8;
}

int main(void) {
    UART settings = {UART_FORWARD_BAUD, 8, 1, 0, 0, 0};
    const char message[] = UART_FORWARD_MESSAGE;
    puts("UART FORWARD SENDER " UART_FORWARD_BUILD_ID " (" UART_FORWARD_STATUS ")");
    if (mos_uopen(&settings)) {
        puts("UART FORWARD FAIL: EMOS UART1 open rejected");
        return 1;
    }
    for (unsigned int n = 0; n < sizeof(message) - 1; ++n) {
        /* EMOS's public call checks its open flag and lifecycle ownership.
         * With flow control disabled it never waits for the absent peer.
         * The inherited driver still polls its own transmitter readiness.
         */
        /* AgonDev b67ab244's libmos/mos_uputc.src inverts the documented
         * carry result: this installed wrapper returns 0 for success, 1 for
         * closed. The paired build checks its linked instructions. Remove
         * this inversion when adopting a corrected upstream wrapper; do not
         * silently trust the contrary comment in release/include/agon/mos.h.
         */
        if (mos_uputc((unsigned char)message[n]) != 0) {
            mos_uclose();
            puts("UART FORWARD FAIL: EMOS UART1 write rejected");
            return 1;
        }
    }
    const int settled = settle_transmitter();
    mos_uclose();
    /* Upstream close resets UART registers but retains PC0/PC1 alternate
     * functions. It is not a GPIO isolation operation (MOS v3.0.2 lineage).
     */
    if (!settled) {
        puts("UART FORWARD FAIL: MOS clock stalled during transmit settling");
        return 1;
    }
    printf("UART FORWARD SENT %u bytes; P4 must confirm receipt\n",
           (unsigned int)(sizeof(message) - 1));
    return 0;
}
