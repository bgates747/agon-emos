/* PORT-017 private F6/8D transport; full contract is owned by agon-extender.
 * The only retained buffers are Core RAM, not transient application pointers.
 * The foreground application validates record CRCs and performs MOS file I/O.
 */
#include <string.h>
#include "ff.h"
#include "emos_sdlink.h"
#include "emos_admission.h"
#include "emos_keyboard.h"
#include "uart.h"

static volatile BYTE active, ready, application_owned, second_ready;
/* A finite job can receive a control ACK followed immediately by its first
 * file request. Keep both bounded packets until foreground consumption; a
 * single mailbox silently loses that legitimate pair. Other owners retain
 * their existing one-packet behavior. IRQ still only copies bytes. */
static BYTE second_mailbox[EMOS_SDLINK_LIMIT];
static UINT32 application_token; /* Never reused within a boot. */
static BYTE mailbox[EMOS_SDLINK_LIMIT], transmit[EMOS_SDLINK_LIMIT + 4];

static BYTE range(UINT24 address, UINT24 length) {
    /* REMOTE-005: foreground MOSlets own B0000..B7FFF too. Keep MOS RAM
     * above B8000 excluded; subtract before comparison to avoid wrapping. */
    return address >= 0x040000 && address < 0x0B8000 && length <= 0x0B8000-address;
}
void emos_sdlink_reset(void) {
    BYTE irq = emos_keyboard_lock();
    active = ready = application_owned = second_ready = 0;
    emos_keyboard_unlock(irq);
}
void emos_sdlink_packet(const BYTE *data, BYTE length) {
    /* Existing UART1 ISR, all registers saved, interrupts disabled. */
    if (!application_owned && length >= 4 && data[3] == 5) { emos_admission_packet(data, length); return; }
    if (active && length >= 20 && length <= EMOS_SDLINK_LIMIT) {
        if(!ready) {
            memcpy(mailbox, data, length);
            ready = length; /* Publish only after the complete record is copied. */
        } else if(application_owned==2 && !second_ready) {
            memcpy(second_mailbox,data,length);second_ready=length;
        }
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
    if (operation > 6 || (operation != 2 && operation != 6 && length != 1) || (operation == 6 && length != 6) ||
        (operation == 2 && (length < 21 || length > EMOS_SDLINK_LIMIT+1)) ||
        (operation == 1 ? capacity < EMOS_SDLINK_LIMIT : operation == 4 ? capacity != 4 : operation == 5 ? capacity != 36 : capacity != 0))
        return FR_INVALID_PARAMETER;
    /* Only the negotiated finite dispatcher may use active ExCom framing.
     * Manual and application-owned services retain their Legacy boundary. */
    if ((emos_get_mode() != EMOS_MODE_LEGACY &&
         !(emos_get_mode()==EMOS_MODE_EXCLUSIVE_COMPAT && emos_admission_excom() &&
           (operation==5 || (application_owned==2 && operation!=0 && operation!=4)))) ||
        emos_key_source != EMOS_KEY_EXTENDER ||
        emos_key_faulted || !uart1_keyboard_owned) return EMOS_UNAVAILABLE;
    irq = emos_keyboard_lock();
    if (!irq) return EMOS_BUSY; /* A service call never runs inside an ISR. */
    if (operation == 5) {
        if(active || !emos_admission_binding((BYTE *)output)) { emos_keyboard_unlock(irq); return EMOS_BUSY; }
        active=1;application_owned=2;ready=second_ready=0;r->outputLength[0]=36;
        emos_keyboard_unlock(irq);return FR_OK;
    }
    if(operation == 6) {
        if(application_owned!=2 || !emos_admission_terminal((BYTE *)input+1)) { emos_keyboard_unlock(irq); return EMOS_BUSY; }
        emos_keyboard_unlock(irq);return FR_OK;
    }
    /* A05: only an executing application/MOSlet can request this lease.
     * It never loads another program. No P4 packet can set application_owned. */
    if (operation == 4) {
        if (active || !emos_application_context()) { emos_keyboard_unlock(irq); return EMOS_BUSY; }
        if (application_token == 0xffffffffUL) { emos_keyboard_unlock(irq); return EMOS_UNAVAILABLE; }
        ++application_token; active = application_owned = 1; ready = 0;
        ((BYTE *)output)[0]=application_token; ((BYTE *)output)[1]=application_token>>8;
        ((BYTE *)output)[2]=application_token>>16; ((BYTE *)output)[3]=application_token>>24;
        r->outputLength[0]=4;
        emos_keyboard_unlock(irq);
        emos_admission_reset();
        return FR_OK;
    }
    if (operation == 0 && application_owned) { emos_keyboard_unlock(irq); return EMOS_BUSY; }
    if (operation == 0 || operation == 3) {
        application_owned = 0;
        active = operation == 0; ready = second_ready = 0;
        emos_keyboard_unlock(irq);
        return FR_OK;
    }
    if (!active) { emos_keyboard_unlock(irq); return EMOS_UNAVAILABLE; }
    if (operation == 1) {
        n = ready;
        if (n) memcpy((BYTE *)output,mailbox,n);
        ready = second_ready;
        if(second_ready)memcpy(mailbox,second_mailbox,second_ready);
        second_ready=0;r->outputLength[0] = n;
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
