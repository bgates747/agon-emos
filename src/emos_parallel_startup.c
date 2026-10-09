/* PORT-008 private boot-release composition, not qualified for deployment.
 * Startup/foreground owns connect; the existing keyboard ISR owns tick.
 * Tick never waits. No application/callback may invoke these private hooks.
 * Never wait inside the UART/keyboard IRQ lock. No timed UART fallback.
 * Existing transport claim/release owns vectors, parser and receive readiness.
 */
#include <ez80.h>
#include "emos_parallel_handover.h"
#include "emos_keyboard.h"
#include "uart.h"
static t_emosParallelHandover parallel_boot_state;
/* Shared with the existing keyboard ISR; admission callers must resample. */
static volatile BYTE parallel_boot_grant;
void emos_parallel_boot_start(void) {
    parallel_boot_grant = 0;
    emos_parallel_boot_fence(&parallel_boot_state);
}
BYTE emos_parallel_boot_uart_allowed(void) {
    /* A later high READY cannot resurrect an observed loss. Grant 1 is the
     * sole reciprocal restore, before the peer raises its UART acknowledgement. */
    if (parallel_boot_grant == 2 && !(PD_DR & 0x10)) parallel_boot_grant = 0;
    return parallel_boot_grant;
}
void emos_parallel_boot_tick(void) {
    /* Caller excludes normal parallel parking. Observed reset is a fault,
     * not a drained suspension: invalidate receive/held keys before fencing. */
    if (uart1_keyboard_owned && !emos_parallel_boot_uart_allowed()) {
        emos_keyboard_fault();
        uart1_keyboard_close();
        emos_key_source = EMOS_KEY_MAINBOARD;
        emos_parallel_boot_start();
    }
}
void emos_parallel_boot_connect(void) {
    t_emosDeadline d;
    BYTE opened = 0, action;
    d.last = emos_keyboard_clock(); d.elapsed = 0; d.budget = (UINT24)262144;
    while (emos_keyboard_deadline_step(&d)) {
        action = emos_parallel_boot_poll(&parallel_boot_state, opened);
        if (action == EPH_RESTORE && !opened) {
            parallel_boot_grant = 1; /* reciprocal release, sole authorized UART restore */
            if (emos_keyboard_transport_claim() != EMOS_KEY_OK) break;
            opened = 1;
        }
        if (action == EPH_LIVE) {
            parallel_boot_grant = 2;
            emos_keyboard_transport_release(); /* startup owns no keyboard source */
            return;
        }
        if (action == EPH_RELEASE) break;
    }
    parallel_boot_grant = 0;
    if (opened) emos_keyboard_transport_release();
    emos_parallel_boot_fence(&parallel_boot_state); /* stay released; mainboard MOS continues */
}
