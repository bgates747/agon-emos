/* PORT-008 LC02: synchronous admitted physical owner.
 * Entry is AFTER exact block ACK under the existing serializer reservation.
 * EMOS owns capability/mode admission; no arbitrary application GPIO bypass
 * is supplied here.
 * Ordinary input is retained through normal parking. Uncertain/faulted work
 * revokes admission and holds the physical fence/reservation until reset.
 */
#include "emos_parallel_handover.h"
#include "emos_parallel.h"
#include "emos_keyboard.h"
#include "uart.h"
#include <ez80.h>
#include "emos_phase_timing.h"
extern BYTE emos_parallel_native_payload(t_emosParallelHandover *, BYTE,
    BYTE, BYTE *, UINT24);
extern void emos_parallel_io_write_control(UINT24);
BYTE emos_parallel_native_coordinate(t_parallelSession *s, const BYTE *offer,
    BYTE *buffer, UINT24 capacity) {
    t_emosParallelHandover *h = emos_parallel_boot_handover();
    t_emosDeadline deadline;
    BYTE phase, done, action, parked = 0, restored = 0, status = 0;
    UINT24 length;
    /* No mutation on refusal, including nested/IRQ-disabled contenders. */
    if (!s || !offer || !buffer || !emos_keyboard_parallel_reserved() ||
        s->phase != PARALLEL_SESSION_ACTIVE || h->phase != EPH_DRAIN ||
        h->failed || !parallel_block_matches(offer,offer,PARALLEL_OFFER) || offer[3] != PARALLEL_OFFER ||
        offer[12] > 1 || offer[13] || memcmp(s->bytes, offer+4, 6)) return 0;
    length = (UINT24)offer[10] | ((UINT24)offer[11] << 8);
    if (!length || length > 4096 || capacity < length ||
        !emos_parallel_boot_block_begin()) return 0;
    PHASE(coordinate_begin);
    phase = h->phase;
    deadline.last = emos_keyboard_clock(); deadline.elapsed = 0;
    deadline.budget = (UINT24)262144;
    while (emos_keyboard_deadline_step(&deadline)) {
        done = (PD_DR & 0x10) ? EPH_READY_HIGH : 0;
        if (emos_key_faulted) break;
        if (h->phase == EPH_DRAIN) {
            BYTE r = emos_keyboard_parallel_park();
            if (r == UART_POLL_EMPTY) continue;
            if (r != UART_POLL_READY) break;
            PHASE(park_end);
            parked = 1; done |= EPH_QUIET;
        }
        if (h->phase == EPH_ENTRY_RELEASE || h->phase == EPH_BLOCK_RELEASE) {
            if (!parked || PC_DDR != 0xFF || PC_ALT1 || PC_ALT2) break;
            done |= EPH_RELEASED;
        }
        if (h->phase == EPH_LOCAL_ARM) done |= EPH_ARMED;
        if (h->phase == EPH_BLOCK) {
            PHASE(payload_begin);
            status = emos_parallel_native_payload(h,parked,offer[12],buffer,length);
            PHASE(payload_end);
            if (status != EMOS_PARALLEL_OK) break;
            done |= EPH_DONE;
        }
        if (h->phase == EPH_RETURN_UART) {
            if (!restored) {
                if (emos_keyboard_parallel_unpark() != UART_POLL_READY) break;
                PHASE(unpark_end);
                restored = 1;
            }
            done |= EPH_UART_UP;
        }
        action = emos_parallel_handover_step(h,done);
        if (h->failed || action == EPH_FENCE || (action == EPH_ARM && h->phase != EPH_LOCAL_ARM))
            break;
        /* Native byte runner owns controls only inside its bounded call. */
        emos_parallel_io_write_control((h->validN ? 0x80 : 0) |
            (h->clockHigh ? 0x20 : 0));
        if (action == EPH_LIVE) {
            /* COMPLETION-REMOVAL-R01: native success, pad release, our UART
             * restore and fresh peer READY-high already establish completion.
             * No second serial COMPLETE/ACK or remote status wait. */
            if (!restored) break;
            emos_parallel_boot_block_end(1);
            /* A late owner fault can still refuse release after physical return.
             * Route that failure through the same physical fence below. */
            if (emos_keyboard_parallel_release() == UART_POLL_READY) return 1;
            break;
        }
        if (phase != h->phase) {
            phase = h->phase; deadline.last = emos_keyboard_clock();
            deadline.elapsed = 0; deadline.budget = (UINT24)262144;
        }
    }
    emos_parallel_session_cancel(h,s);
    emos_parallel_boot_block_end(0);
    return 0; /* Destination stays provisional; reset required after fault. */
}
