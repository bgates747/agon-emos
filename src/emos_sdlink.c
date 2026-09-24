/* PORT-017 private F6/8D transport; full contract is owned by agon-extender.
 * The only retained buffers are Core RAM, not transient application pointers.
 * The foreground application validates record CRCs and performs MOS file I/O.
 */
#include <string.h>
#include "ff.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"

static volatile BYTE active, ready;
static BYTE mailbox[EMOS_SDLINK_LIMIT], transmit[EMOS_SDLINK_LIMIT + 4];

static BYTE range(UINT24 address, UINT24 length) {
    /* REMOTE-005: foreground MOSlets own B0000..B7FFF too. Keep MOS RAM
     * above B8000 excluded; subtract before comparison to avoid wrapping. */
    return address >= 0x040000 && address < 0x0B8000 && length <= 0x0B8000-address;
}
void emos_sdlink_reset(void) {
    BYTE irq = emos_keyboard_lock();
    active = ready = 0;
    emos_keyboard_unlock(irq);
}
void emos_sdlink_packet(const BYTE *data, BYTE length) {
    /* Existing UART1 ISR, all registers saved, interrupts disabled. */
    if (active && !ready && length >= 20 && length <= EMOS_SDLINK_LIMIT) {
        memcpy(mailbox, data, length);
        ready = length; /* Publish only after the complete record is copied. */
    }
}
UINT24 emos_sdlink_gateway(t_emosGatewayRequest *r) {
    UINT24 input, length, output, capacity;
    BYTE operation, irq, n, result;
    if (!range((UINT24)r, sizeof(*r))) return FR_INVALID_PARAMETER;
    input = emos_read24(r->input); length = emos_read24(r->inputLength);
    output = emos_read24(r->output); capacity = emos_read24(r->outputCapacity);
    if (!range(input,length) || !length ||
        (capacity ? !range(output,capacity) : output != 0)) return FR_INVALID_PARAMETER;
    memset(r->outputLength, 0, 3);
    operation = *(BYTE *)input;
    if (operation > 3 || (operation != 2 && length != 1) ||
        (operation == 2 && (length < 21 || length > EMOS_SDLINK_LIMIT+1)) ||
        (operation == 1 ? capacity < EMOS_SDLINK_LIMIT : capacity != 0))
        return FR_INVALID_PARAMETER;
    if (emos_get_mode() != EMOS_MODE_LEGACY || emos_key_source != EMOS_KEY_EXTENDER ||
        emos_key_faulted || !uart1_keyboard_owned) return EMOS_UNAVAILABLE;
    irq = emos_keyboard_lock();
    if (!irq) return EMOS_BUSY; /* A service call never runs inside an ISR. */
    if (operation == 0 || operation == 3) {
        active = operation == 0; ready = 0;
        emos_keyboard_unlock(irq);
        return FR_OK;
    }
    if (!active) { emos_keyboard_unlock(irq); return EMOS_UNAVAILABLE; }
    if (operation == 1) {
        n = ready;
        if (n) memcpy((BYTE *)output,mailbox,n);
        ready = 0; r->outputLength[0] = n;
        emos_keyboard_unlock(irq);
        return FR_OK;
    }
    emos_keyboard_unlock(irq);
    transmit[0]=23; transmit[1]=0; transmit[2]=0xF6;
    transmit[3]=(BYTE)(length-1);
    memcpy(transmit+4,(BYTE *)input+1,length-1);
    result = emos_keyboard_send(transmit,(UINT16)(length+3));
    return result == EMOS_KEY_OK ? FR_OK : FR_TIMEOUT;
}
