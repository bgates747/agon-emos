/* PORT-008: UARTFLOW's sequence/reporting moved to projects/uartflow.
 * This retained source/profile name now contains only the admitted transport
 * leaves. API 0x51 ext.uartdiag v1: input[op,arg], output[poll_status,value].
 * Ops: 0=open fixed 1152000/8N1/CTS+RTS, 1=RX, 2=TX, 3=RTS ready, 4=close.
 * Caller policy/Legacy gate belongs to emos_gateway. No caller pointer lives
 * beyond one call. Utility exit closes our lease, never somebody else's UART.
 */
#include "emos_uart_flow.h"
#include "emos_keyboard.h"
#include "uart.h"
#include "ff.h"
static BYTE active;
static BYTE pair_range(UINT24 p) {
    return p >= EMOS_MODULE_BASE && p <= EMOS_MODULE_BASE+EMOS_MODULE_SIZE-2;
}
void emos_uartdiag_reset(void) {
    BYTE irq=emos_keyboard_lock();
    if(active) { close_UART1(); active=0; }
    emos_keyboard_unlock(irq);
}
UINT24 emos_uartdiag_gateway(t_emosGatewayRequest *r) {
    UINT24 in=emos_read24(r->input),out=emos_read24(r->output);
    BYTE op,arg,status=UART_POLL_UNAVAILABLE,value=0,irq;
    UART settings={1152000,8,1,0,FCTL_HW,0};
    if(!pair_range(in)||!pair_range(out)||emos_read24(r->inputLength)!=2 ||
       emos_read24(r->outputCapacity)!=2) return FR_INVALID_PARAMETER;
    op=*(BYTE *)in;arg=*((BYTE *)in+1);
    if(op>4 || (op!=2 && (op==3 ? arg>1 : arg!=0))) return FR_INVALID_PARAMETER;
    irq=emos_keyboard_lock();
    if(!irq){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    if(op==0) {
        if(!active && !uart1_keyboard_owned && !(serialFlags&0x10) &&
           open_UART1(&settings)==UART_ERR_NONE) {
            active=1;status=uart1_claim_rts();
            if(status!=UART_POLL_READY)emos_uartdiag_reset();
        }
    } else if(active) {
        if(op==1)status=uart1_try_get(&value);
        else if(op==2)status=uart1_try_put(arg);
        else if(op==3)status=uart1_receive_ready(arg);
        else {emos_uartdiag_reset();status=UART_POLL_READY;}
    }
    emos_keyboard_unlock(irq);
    ((BYTE *)out)[0]=status;((BYTE *)out)[1]=value;
    r->outputLength[0]=2;r->outputLength[1]=r->outputLength[2]=0;
    return FR_OK;
}
