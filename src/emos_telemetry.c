#ifdef EMOS_BENCH_TELEMETRY
/* BENCH-001 private Legacy telemetry publication. See Extender's TELEMETRY.md.
 * The gateway is foreground-only; Core's UART IRQ owns deferred transmission.
 * Deliberately no filesystem work and no dynamic provider/application callback. */
#include <string.h>
#include "ff.h"
#include "emos_telemetry.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"
static volatile BYTE active;
static BYTE range(UINT24 address,UINT24 length) {
    return address >= 0x040000 && address < 0x0B0000 && length <= 0x0B0000-address;
}
void emos_telemetry_reset(void) { active=0; }
UINT24 emos_telemetry_gateway(t_emosGatewayRequest *r) {
    UINT24 input,length;
    BYTE operation,irq,result;
    if (!range((UINT24)r,sizeof(*r))) return FR_INVALID_PARAMETER;
    input=emos_read24(r->input); length=emos_read24(r->inputLength);
    if (!length || !range(input,length) ||
        (r->output[0] | r->output[1] | r->output[2] |
         r->outputCapacity[0] | r->outputCapacity[1] | r->outputCapacity[2]))
        return FR_INVALID_PARAMETER;
    r->outputLength[0]=r->outputLength[1]=r->outputLength[2]=0;
    operation=*(BYTE *)input;
    if (operation>2 || length != (operation==1 ? 141 : 1)) return FR_INVALID_PARAMETER;
    if (emos_get_mode()!=EMOS_MODE_LEGACY || emos_key_source!=EMOS_KEY_EXTENDER ||
        emos_key_faulted || !uart1_keyboard_owned) return EMOS_UNAVAILABLE;
    irq=emos_keyboard_lock();
    if (!irq) return EMOS_BUSY;
    if (operation!=1) {
        active=operation==0;
        emos_keyboard_unlock(irq); return FR_OK;
    }
    if (!active) { emos_keyboard_unlock(irq); return EMOS_UNAVAILABLE; }
    emos_keyboard_unlock(irq);
    result=emos_keyboard_queue_telemetry((BYTE *)input+1);
    return result==EMOS_KEY_OK ? FR_OK : EMOS_BUSY;
}

#endif
