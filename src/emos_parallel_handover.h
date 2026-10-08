/* PORT-008 F02c1 / INTEG-016: private, unbound ownership sequencer.
 * Caller holds the existing lifecycle/writer lock, serializes UART/ISR users,
 * and supplies COMPLETED adapter actions, never optimistic requests. No API,
 * mode change, pin binding, wire decoder or autonomous admission lives here.
 * See Extender docs/tasks/PORT-008/HANDOVER.md for the paired truth table.
 */
#ifndef EMOS_PARALLEL_HANDOVER_H
#define EMOS_PARALLEL_HANDOVER_H
#include <defines.h>

#define EPH_READY_HIGH 1
#define EPH_QUIET 2       /* fenced TX queue + shift register empty, RX boundary */
#define EPH_RELEASED 4    /* peripheral stopped, ALL eight output enables off */
#define EPH_ARMED 8
#define EPH_DONE 16       /* block runner returned, not DMA completion alone */
#define EPH_UART_UP 32

#define EPH_WAIT 0
#define EPH_FENCE 1
#define EPH_RELEASE 2
#define EPH_ARM 3
#define EPH_RUN 4
#define EPH_RESTORE 5
#define EPH_LIVE 6

#define EPH_RETURN_RELEASE 0
#define EPH_RETURN_ACK 1
#define EPH_RETURN_REQUEST 13
#define EPH_RETURN_UART 2
#define EPH_RETURN_PEER 3
#define EPH_UART 4
#define EPH_DRAIN 5
#define EPH_ENTRY_RELEASE 6
#define EPH_ENTRY_ACK 7
#define EPH_ENTRY_CLEAR 8
#define EPH_ENTRY_ARM 9
#define EPH_LOCAL_ARM 10
#define EPH_BLOCK 11
#define EPH_BLOCK_RELEASE 12

typedef struct {
    BYTE phase;
    BYTE validN;
    BYTE clockHigh;
    BYTE failed;
} t_emosParallelHandover;

/* init/cancel request release; they do NOT falsely acknowledge it. A reset
 * adapter must leave shared pads as inputs before publishing these controls.
 * No waits inside step: the caller owns a real deadline and stalled-clock fuse.
 * After cancellation/timeout, completion is a failure even if UART recovers.
 */
void emos_parallel_handover_init(t_emosParallelHandover *h);
void emos_parallel_handover_cancel(t_emosParallelHandover *h);
/* Only after coordinator-owned matching ExExt admission, at a UART boundary.
 * Clear prior input acknowledgements when starting each new adapter action.
 * Block RUN temporarily owns CLOCK/VALID until it returns EPH_DONE.
 */
BYTE emos_parallel_handover_begin(t_emosParallelHandover *h);
BYTE emos_parallel_handover_step(t_emosParallelHandover *h, BYTE completed);
#endif
