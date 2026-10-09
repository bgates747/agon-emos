/* PORT-008 F02c3 / INTEG-016: private unbound native payload leaf.
 * Selected ONLY by parallel-native-candidate.mk. The foreground coordinator
 * retains its admission, lifecycle lock, buffer and UART parking lease. This
 * leaf never negotiates, waits for READY, restores UART or commits a mode.
 * RX bytes remain provisional until matched post-block UART status succeeds.
 * The assembly symbol below is private to this translation unit's binding;
 * no MOS API, CLI command or standalone raw-GPIO entry is supported.
 */
#include "emos_parallel_handover.h"
#include "emos_parallel.h"
#include <ez80.h>
#include "uart.h"
extern BYTE emos_parallel_native_bytes(BYTE *buffer, UINT24 length, BYTE reverse);
BYTE emos_parallel_native_payload(t_emosParallelHandover *h, BYTE parked,
    BYTE reverse, BYTE *buffer, UINT24 length) {
    BYTE status;
    if (!h || !buffer || !length || length > 4096 || reverse > 1)
        return EMOS_PARALLEL_INVALID;
    if (parked != 1 || h->phase != EPH_BLOCK || h->failed ||
        !h->validN || !h->clockHigh || UART1_IER ||
        UART1_MCTL != UART_MCTL_LOOP || PC_DDR != 0xFF ||
        PC_ALT1 || PC_ALT2 || (PD_DDR & 0xB0) != 0x10 ||
        (PD_ALT1 & 0xB0) || (PD_ALT2 & 0xB0) || (PD_DR & 0xB0) != 0xA0)
        return EMOS_PARALLEL_NOT_OWNED;
    status = emos_parallel_native_bytes(buffer, length, reverse);
    if (status != EMOS_PARALLEL_OK) h->failed = 1;
    return status;
}
