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
extern void emos_parallel_io_write_control(UINT24 bits);
static t_emosParallelHandover parallel_boot_state;
/* Shared with the existing keyboard ISR; admission callers must resample. */
static volatile BYTE parallel_boot_grant;
#if defined(EMOS_PARALLEL_NATIVE_COORDINATOR)
static volatile BYTE parallel_boot_block;
#endif
void emos_parallel_boot_start(void) {
    parallel_boot_grant = 0;
#if defined(EMOS_PARALLEL_NATIVE_COORDINATOR)
    parallel_boot_block = 0;
#endif
    emos_parallel_boot_fence(&parallel_boot_state);
    /* A mainboard-only reset otherwise leaves C0/V1 identical to live UART:
     * old READY-high can be mistaken for a new release acknowledgement before
     * P4 observes VALID-low. Request recovery first, with our data pads already
     * released. P4 withdraws READY only after its own pad fence completes.
     * This is not a UART grant; connect still requires the full release cycle. */
    emos_parallel_io_write_control(0x20); /* C1/V0: reset request, no data drive */
}
BYTE emos_parallel_boot_uart_allowed(void) {
    /* A later high READY cannot resurrect an observed loss. Grant 1 is the
     * sole reciprocal restore, before the peer raises its UART acknowledgement. */
#if defined(EMOS_PARALLEL_NATIVE_COORDINATOR)
    /* Normal return has READY low until P4 restores its UART. Only the
     * reserved foreground owner may unpark at this proven return phase. */
    if (parallel_boot_block) return parallel_boot_state.phase == EPH_RETURN_UART;
#endif
    if (parallel_boot_grant == 2 && !(PD_DR & 0x10)) parallel_boot_grant = 0;
    return parallel_boot_grant;
}
void emos_parallel_boot_tick(void) {
    /* Caller excludes normal parallel parking. Observed reset is a fault,
     * not a drained suspension: invalidate receive/held keys before fencing. */
#if defined(EMOS_PARALLEL_NATIVE_COORDINATOR)
    if (parallel_boot_block) return;
#endif
    if (uart1_keyboard_owned && !emos_parallel_boot_uart_allowed()) {
        emos_keyboard_fault();
        uart1_keyboard_close();
        emos_key_source = EMOS_KEY_MAINBOARD;
        emos_parallel_boot_start();
    }
}
void emos_parallel_boot_connect(void) {
    t_emosDeadline d;
    BYTE opened = 0, action, requested = 0;
    d.last = emos_keyboard_clock(); d.elapsed = 0; d.budget = (UINT24)262144;
    while (emos_keyboard_deadline_step(&d)) {
        if (requested == 0) {
            if (!(PD_DR & 0x10)) continue;
            /* An old READY-high is allowed here: CLOCK stays high, so the
             * peer must still observe the request and acknowledge C1/V1 low. */
            emos_parallel_io_write_control(0xA0); /* C1/V1 */
            requested = 1;
            continue;
        }
        if (requested == 1) {
            if (PD_DR & 0x10) continue;
            /* Fresh peer recovery (or an unfinished old block) alone grants
             * nothing. Release C0/V1 and require high/low/high plus actual
             * local UART restore through the established phase machine. */
            emos_parallel_boot_fence(&parallel_boot_state);
            requested = 2;
        }
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

#if defined(EMOS_PARALLEL_NATIVE_COORDINATOR)
/* LC02 private foreground ownership. No MOS API, CLI or raw GPIO caller.
 * A fault keeps the UART reservation parked/fenced until reset; an ordinary
 * timeout cannot revive a suspended receiver or replay an uncertain block. */
t_emosParallelHandover *emos_parallel_boot_handover(void) {
    return &parallel_boot_state;
}
BYTE emos_parallel_boot_block_begin(void) {
    BYTE irq = emos_keyboard_lock(), ok;
    ok = irq && !parallel_boot_block && parallel_boot_grant == 2 &&
        parallel_boot_state.phase == EPH_DRAIN && !parallel_boot_state.failed;
    if (ok) parallel_boot_block = 1;
    emos_keyboard_unlock(irq);
    return ok;
}
void emos_parallel_boot_block_end(BYTE success) {
    BYTE irq = emos_keyboard_lock();
    if (success && parallel_boot_state.phase == EPH_UART &&
        !parallel_boot_state.failed) parallel_boot_block = 0;
    else emos_parallel_boot_start(); /* physical input fence; never unpark */
    emos_keyboard_unlock(irq);
}

#endif
