#include "emos_parallel_handover.h"

/* PORT-008 boot-only physical binding, deliberately no startup caller yet.
 * Reset/coordinator owns an exclusive fence: UART ISR and all competing
 * writers are inactive. Normal live suspension uses existing parking/drain.
 * Port D helpers preserve IFF and unrelated onboard UART0/SD GPIO bits.
 */
#include <ez80.h>
#include "uart.h"
extern void emos_parallel_io_configure(UINT24 ddr, UINT24 alt1, UINT24 alt2);
extern void emos_parallel_io_write_control(UINT24 bits);
void emos_parallel_boot_fence(t_emosParallelHandover *h) {
    UART1_IER = 0;
    PC_DDR = 0xFF;
    PC_ALT1 = 0;
    PC_ALT2 = 0;
    UART1_MCTL = UART_MCTL_LOOP; /* disconnected external RX, not queue drain */
    emos_parallel_io_configure(0xB0, 0, 0); /* all controls input first */
    emos_parallel_io_write_control(0x80); /* C=0,V=1 BEFORE enabling outputs */
    emos_parallel_io_configure(0x10, 0, 0); /* READY input, CLOCK/VALID output */
    emos_parallel_handover_init(h);
}
BYTE emos_parallel_boot_poll(t_emosParallelHandover *h, BYTE uartRestored) {
    BYTE done, action;
    /* Caller must start a new exclusive fence after a peer reset/live fault;
     * never reuse the boot's old RELEASED bit while UART outputs are active. */
    if (h->phase > EPH_UART && h->phase != EPH_RETURN_REQUEST) return EPH_RELEASE;
    done = (PD_DR & 0x10) ? EPH_READY_HIGH : 0;
    if (h->phase == EPH_RETURN_RELEASE && PC_DDR == 0xFF &&
        PC_ALT1 == 0 && PC_ALT2 == 0) done |= EPH_RELEASED;
    if (h->phase == EPH_RETURN_UART && uartRestored) done |= EPH_UART_UP;
    action = emos_parallel_handover_step(h, done);
    /* A RELEASE request is NOT permission to advertise a new release. */
    if (action != EPH_RELEASE)
        emos_parallel_io_write_control(h->validN ? 0x80 : 0);
    return action;
}
