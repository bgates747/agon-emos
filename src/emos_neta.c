/* INTEG-024: EMOS owns admission/RX/LSR/CTS/IER and Port C exclusion.
 * The admitted ordinary-C hook, core, source/count/callback and state live in
 * application RAM. No resident serializer or generic-driver copy lives here.
 * Hook event START1 returns admission; SERVICE0/FAULT2 return demand:
 * IDLE0/TX1/CTS-RETRY2/TAIL3/FAULT4. All calls are under owner exclusion.
 * LSR is the existing RX drain's SAME acknowledging sample, DR replaced by CTS
 * STOP bit. Never sample LSR separately and lose an RX/error event.
 */
#include "emos_neta.h"
#include "emos_keyboard.h"
#include "emos_sdlink.h"
#include "uart.h"
#include "ff.h"
typedef BYTE (*Hook)(BYTE,BYTE);
static Hook hook;
static BYTE demand;
extern BYTE emos_neta_cts(void);
extern void uart1_keyboard_irq(void);
static void irq_demand(void){
    /* RX/error enables stay owned by EMOS. GPIO CTS has no UART modem IRQ. */
    if(demand==1)UART1_IER|=UART_IER_TRANSMITINT;
    else UART1_IER&=(BYTE)~UART_IER_TRANSMITINT;
}
BYTE emos_neta_open(UINT24 entry){
    if(hook)return EMOS_BUSY;
    if(!emos_application_span(entry,1))return FR_INVALID_PARAMETER;
    if(emos_keyboard_app_reserve()!=UART_POLL_READY)return EMOS_BUSY;
    hook=(Hook)entry;demand=0;return FR_OK;
}
void emos_neta_fault(BYTE lsr){
    if(hook){(void)hook(2,lsr&0x9e);demand=4;irq_demand();}
}
void emos_neta_service(BYTE lsr){
    if(!hook)return;
    if(uart1_keyboard_owned!=1||emos_key_faulted){emos_neta_fault(0);return;}
    if(lsr&0x9e){emos_neta_fault(lsr);return;}
    demand=hook(0,(lsr&0xfe)|(emos_neta_cts()!=0));
    if(demand>4)demand=4;
    irq_demand();
}
BYTE emos_neta_progress(void){
    if(!hook)return EMOS_UNAVAILABLE;
    if(uart1_keyboard_owned!=1||emos_key_faulted){emos_neta_fault(0);return EMOS_UNAVAILABLE;}
    uart1_keyboard_irq(); /* bounded RX first; shared final clean sample */
    return demand==0?FR_OK:demand==4?EMOS_UNAVAILABLE:EMOS_BUSY;
}
BYTE emos_neta_start(void){
    BYTE r;
    if(!hook)return EMOS_UNAVAILABLE;
    if(demand)return demand==4?EMOS_UNAVAILABLE:EMOS_BUSY;
    r=hook(1,0);if(r)return r;
    demand=1;irq_demand();
    /* Accepted source remains accepted even if this first service faults. */
    (void)emos_neta_progress();return FR_OK;
}
BYTE emos_neta_close(void){
    BYTE r=emos_neta_progress();if(r)return r;
    hook=0;demand=0;irq_demand();emos_keyboard_app_release(0);return FR_OK;
}
void emos_neta_abort(void){
    BYTE irq=emos_keyboard_lock(),live=demand!=0;
    if(hook){
        if(live)(void)hook(2,0); /* retain first app fault, never source release */
        hook=0;demand=0;irq_demand();emos_keyboard_app_release(live);
    }
    emos_keyboard_unlock(irq);
}
void emos_neta_tick(void){if(hook&&demand&&demand!=4)(void)emos_neta_progress();}
