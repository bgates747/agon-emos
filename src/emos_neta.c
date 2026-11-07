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
extern void uart1_keyboard_irq(void);
/* Private native IRQ admission reads only its admitted high address byte. */
Hook emos_neta_hook;
static BYTE demand;
#ifdef EMOS_UART1_RX_DIAGNOSTIC
volatile UINT24 emos_neta_diag_hook;
#endif
/* RX-R01: keep peer STOP across the complete foreground gateway exclusion,
 * not merely while its nested RX drain runs. Engine and buffers stay in app. */
volatile BYTE emos_neta_rx_hold;
/* Distinct owners: UART0 must never clear a foreground hold. A capped drain
 * is not empty; keep STOP until the existing RX path sees clean-empty. */
volatile BYTE emos_neta_uart0_hold, emos_neta_rx_deferred;
BYTE emos_neta_rx_ready(void){
    return !emos_neta_rx_hold&&!emos_neta_uart0_hold&&!emos_neta_rx_deferred&&
        demand!=4&&emos_keyboard_app_rx_ready();
}
BYTE emos_neta_uart0_rx(void){
    uart1_keyboard_irq();
    emos_neta_uart0_hold=0;
    return emos_neta_rx_ready();
}
BYTE emos_neta_rx_resume(void){
    return emos_neta_rx_hold&&!emos_neta_uart0_hold&&!emos_neta_rx_deferred&&
        demand!=4&&emos_keyboard_app_rx_ready();
}
extern BYTE emos_neta_cts(void);
/* Native SDK peripheral names are numeric addresses, not C lvalues.
 * Use one bounded maintained IN0/OUT0 leaf, preserving every RX/error enable. */
extern void emos_neta_tx_irq(BYTE);
static void irq_demand(void){emos_neta_tx_irq(demand==1);}
BYTE emos_neta_open(UINT24 entry){
    if(emos_neta_hook)return EMOS_BUSY;
    if(!emos_application_span(entry,1))return FR_INVALID_PARAMETER;
    if(emos_keyboard_app_reserve()!=UART_POLL_READY)return EMOS_BUSY;
    emos_neta_hook=(Hook)entry;demand=0;return FR_OK;
}
void emos_neta_fault(BYTE lsr){
    if(emos_neta_hook){(void)emos_neta_hook(2,lsr&0x9e);demand=4;irq_demand();}
}
void emos_neta_service(BYTE lsr){
    if(!emos_neta_hook)return;
    if(uart1_keyboard_owned!=1||emos_key_faulted){emos_neta_fault(0);return;}
    if(lsr&0x9e){emos_neta_fault(lsr);return;}
    demand=emos_neta_hook(0,(lsr&0xfe)|(emos_neta_cts()!=0));
    if(demand>4)demand=4;
    irq_demand();
}
BYTE emos_neta_progress(void){
    if(!emos_neta_hook)return EMOS_UNAVAILABLE;
    if(uart1_keyboard_owned!=1||emos_key_faulted){emos_neta_fault(0);return EMOS_UNAVAILABLE;}
    uart1_keyboard_irq(); /* bounded RX first; shared final clean sample */
    /* RX may fault/reset and remove the hook synchronously. Abort clears
     * demand, which must not manufacture idle or release the guard twice. */
    if(!emos_neta_hook||uart1_keyboard_owned!=1||emos_key_faulted)return EMOS_UNAVAILABLE;
    return demand==4?EMOS_UNAVAILABLE:(demand||emos_neta_rx_deferred)?EMOS_BUSY:FR_OK;
}
BYTE emos_neta_start(void){
    BYTE r;
    if(!emos_neta_hook)return EMOS_UNAVAILABLE;
    if(demand)return demand==4?EMOS_UNAVAILABLE:EMOS_BUSY;
    r=emos_neta_hook(1,0);if(r)return r;
    demand=1;irq_demand();
    /* Accepted source remains accepted even if this first service faults. */
    (void)emos_neta_progress();return FR_OK;
}
BYTE emos_neta_close(void){
    BYTE r=emos_neta_progress();if(r)return r;
    emos_neta_hook=0;
#ifdef EMOS_UART1_RX_DIAGNOSTIC
    emos_neta_diag_hook=0;
#endif
    demand=0;irq_demand();emos_keyboard_app_release(0);return FR_OK;
}
void emos_neta_abort(void){
    BYTE irq=emos_keyboard_lock(),live=demand!=0||emos_neta_rx_deferred;
    if(emos_neta_hook){
        if(live)(void)emos_neta_hook(2,0); /* retain first app fault, never source release */
        emos_neta_hook=0;
#ifdef EMOS_UART1_RX_DIAGNOSTIC
        emos_neta_diag_hook=0;
#endif
        demand=0;irq_demand();emos_keyboard_app_release(live);
    }
    emos_keyboard_unlock(irq);
}
void emos_neta_tick(void){if(emos_neta_hook&&demand!=4&&(demand||emos_neta_rx_deferred))(void)emos_neta_progress();}
