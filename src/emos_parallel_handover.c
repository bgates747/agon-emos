#include "emos_parallel_handover.h"

void emos_parallel_handover_init(t_emosParallelHandover *h) {
    h->phase = EPH_RETURN_RELEASE;
    h->validN = 1;
    h->clockHigh = 0;
    h->failed = 1; /* a boot/recovery is never a successful payload */
}
void emos_parallel_handover_cancel(t_emosParallelHandover *h) {
    h->failed = 1;
    h->phase = EPH_RETURN_RELEASE;
    /* Keep control levels until the adapter actually stops/releases pads. */
}
BYTE emos_parallel_handover_begin(t_emosParallelHandover *h) {
    if (h->phase != EPH_UART) return 0;
    h->failed = 0;
    h->phase = EPH_DRAIN;
    return 1;
}
BYTE emos_parallel_handover_step(t_emosParallelHandover *h, BYTE done) {
    switch (h->phase) {
    case EPH_UART:
        if (done & EPH_READY_HIGH) return EPH_LIVE;
        /* Unsolicited READY low is a peer reset/recovery, not block permission. */
        emos_parallel_handover_cancel(h);
        return EPH_RELEASE;
    case EPH_DRAIN:
        if (!(done & EPH_QUIET)) return EPH_FENCE;
        h->phase = EPH_ENTRY_RELEASE;
        return EPH_RELEASE;
    case EPH_ENTRY_RELEASE:
        if (!(done & EPH_RELEASED)) return EPH_RELEASE;
        h->clockHigh = 0;
        h->validN = 0;
        h->phase = EPH_ENTRY_ACK;
        break;
    case EPH_ENTRY_ACK:
        if (done & EPH_READY_HIGH) break;
        h->validN = 1;
        h->phase = EPH_ENTRY_CLEAR;
        break;
    case EPH_ENTRY_CLEAR:
        if (!(done & EPH_READY_HIGH)) break;
        h->clockHigh = 1;
        h->phase = EPH_ENTRY_ARM;
        break;
    case EPH_ENTRY_ARM:
        if (done & EPH_READY_HIGH) break;
        h->phase = EPH_LOCAL_ARM;
        return EPH_ARM;
    case EPH_LOCAL_ARM:
        if (!(done & EPH_ARMED)) return EPH_ARM;
        h->phase = EPH_BLOCK;
        return EPH_RUN;
    case EPH_BLOCK:
        if (!(done & EPH_DONE)) break;
        h->phase = EPH_BLOCK_RELEASE;
        return EPH_RELEASE;
    case EPH_BLOCK_RELEASE:
    case EPH_RETURN_RELEASE:
        if (!(done & EPH_RELEASED)) return EPH_RELEASE;
        h->clockHigh = 0;
        h->validN = 1;
        h->phase = EPH_RETURN_ACK;
        break;
    case EPH_RETURN_ACK:
        if (!(done & EPH_READY_HIGH)) break;
        h->validN = 0;
        h->phase = EPH_RETURN_REQUEST;
        break;
    case EPH_RETURN_REQUEST:
        if (done & EPH_READY_HIGH) break;
        h->phase = EPH_RETURN_UART;
        return EPH_RESTORE;
    case EPH_RETURN_UART:
        if (!(done & EPH_UART_UP)) return EPH_RESTORE;
        h->validN = 1;
        h->phase = EPH_RETURN_PEER;
        break;
    case EPH_RETURN_PEER:
        if (!(done & EPH_READY_HIGH)) break;
        h->validN = 1;
        h->phase = EPH_UART;
        return EPH_LIVE;
    default:
        emos_parallel_handover_cancel(h);
        return EPH_RELEASE;
    }
    return EPH_WAIT;
}
