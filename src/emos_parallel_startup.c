/* PORT-008 private boot-release composition, not qualified for deployment.
 * Startup-only single owner; no application, callback or ISR may invoke these
 * hooks. Never wait inside the UART/keyboard IRQ lock. No timed UART fallback.
 * Existing transport claim/release owns vectors, parser and receive readiness.
 */
#include <ez80.h>
#include "emos_parallel_handover.h"
#include "emos_keyboard.h"
static t_emosParallelHandover parallel_boot_state;
static BYTE parallel_boot_grant;
void emos_parallel_boot_start(void) {
    parallel_boot_grant = 0;
    emos_parallel_boot_fence(&parallel_boot_state);
}
BYTE emos_parallel_boot_uart_allowed(void) {
    return parallel_boot_grant && (parallel_boot_grant == 1 || (PD_DR & 0x10));
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
