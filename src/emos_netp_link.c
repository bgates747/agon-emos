/* INTEG-020 Legacy-only service on the existing private bounded-block engine.
 * Capability3 is the historical EX block capability (Legacy probes also use
 * it), NOT a display-route commit. netp tags direct its bytes to the network
 * mailbox instead of the VDU parser. All application payload uses parallel.
 */
#ifdef EMOS_NETWORK_PARALLEL
#include <string.h>
#include "ff.h"
#include "uart.h"
#include "emos_keyboard.h"
#include "emos_parallel.h"
#include "emos_parallel_handover.h"
#include "emos_netp_transport.h"
#include "emos_netp_control.h"
#if NETP_CONTROL_VERSION != PARALLEL_WIRE_VERSION
#error NP local EX view requires the same format version
#endif
extern BYTE emos_netp_control(BYTE *);
static t_parallelSession session;
static BYTE transaction[4],unusable;
UINT32 emos_netp_token;
#define token emos_netp_token
extern BYTE emos_uart0_async_owned(void);
static BYTE reserve(void){
    return !unusable && !emos_uart0_async_owned() && emos_parallel_boot_uart_allowed() &&
        emos_parallel_boot_handover()->phase==EPH_UART &&
        emos_keyboard_parallel_reserve()==UART_POLL_READY;
}
static BYTE fail(void){
    unusable=1;emos_parallel_session_cancel(emos_parallel_boot_handover(),&session);
    emos_parallel_boot_block_end(0);return EMOS_NETP_FAULT;
}
static BYTE renew(void){
    BYTE i;
    for(i=0;i<4;++i)if(++transaction[i])break;
    if(i==4)return 0; /* no transaction reuse at wrap */
    return emos_parallel_session_negotiate(emos_parallel_boot_handover(),&session,transaction);
}
static void control(BYTE *p,BYTE op,BYTE count,BYTE reverse){
    UINT16 seq=(UINT16)(session.bytes[4]|((UINT16)session.bytes[5]<<8))+1;
    memset(p,0,16);memcpy(p,"NP",2);p[2]=NETP_CONTROL_VERSION;p[3]=op;
    memcpy(p+4,&token,4);
    if(op==NETP_CONTROL_SUBMIT||op==NETP_CONTROL_TAKE){p[8]=(BYTE)seq;p[9]=(BYTE)(seq>>8);}
    p[10]=(BYTE)count;p[12]=reverse;
}
BYTE emos_netp_link_open(void){
    BYTE p[16];
    if(token)return EMOS_BUSY;
    if(!reserve())return unusable?EMOS_NETP_FAULT:EMOS_BUSY;
    if(!renew())return fail();
    control(p,NETP_CONTROL_OPEN,0,0);
    if(!emos_netp_control(p))return fail();
    if(p[13]){if(emos_keyboard_parallel_release()!=UART_POLL_READY)return fail();return p[13];}
    memcpy(&token,p+4,4);
    if(!token)return fail();
    if(emos_keyboard_parallel_release()!=UART_POLL_READY)return fail();
    return FR_OK;
}
static BYTE transfer(BYTE *data,BYTE capacity,UINT16 *length,BYTE reverse){
    BYTE p[16],offer[16],ack[16];BYTE actual;
    if(!reserve())return unusable?EMOS_NETP_FAULT:EMOS_BUSY;
    if(session.bytes[4]==255&&session.bytes[5]==255&&!renew())return fail();
    control(p,reverse?NETP_CONTROL_TAKE:NETP_CONTROL_SUBMIT,capacity,reverse);
    if(!emos_netp_control(p))return fail();
    actual=p[10];*length=actual;
    if(p[13]){
        if(emos_keyboard_parallel_release()!=UART_POLL_READY)return fail();
        return p[13];
    }
    if(actual<14||actual>capacity||actual>EMOS_NETP_MAX)return fail();
    /* The acknowledged NP format4 already has the same sequence/count,
     * direction and zeroed reserved bytes as the existing local EX view. */
    memcpy(offer,p,16);memcpy(offer,"EX",2);offer[3]=PARALLEL_OFFER;
    memcpy(offer+4,session.bytes,4);
    memcpy(ack,offer,16);ack[3]=PARALLEL_ACK;
    /* NP ACK is the single payload admission. Reuse the exact existing local
     * block transition without a second wire OFFER/ACK or per-byte handshake. */
    if(!emos_parallel_session_admit(emos_parallel_boot_handover(),PARALLEL_EXEXT,&session,offer,ack)||
       !emos_parallel_native_coordinate(&session,offer,data,capacity))return fail();
    return FR_OK;
}
BYTE emos_netp_link_submit(const BYTE*record,UINT16 length){
    UINT16 actual=0;if(!token)return EMOS_UNAVAILABLE;
    return transfer((BYTE*)record,length,&actual,0);
}
BYTE emos_netp_link_take(BYTE*out,UINT16 capacity,UINT16*length){
    if(!token)return EMOS_UNAVAILABLE;
    return transfer(out,capacity,length,1);
}
BYTE emos_netp_link_release(void){
    BYTE p[16],result;
    if(!token)return EMOS_UNAVAILABLE;
    if(!reserve())return unusable?EMOS_NETP_FAULT:EMOS_BUSY;
    control(p,NETP_CONTROL_RELEASE,0,0);
    if(!emos_netp_control(p))return fail();
    result=p[13];
    if(emos_keyboard_parallel_release()!=UART_POLL_READY)return fail();
    if(!result){token=0;parallel_session_invalidate(&session);}
    return result;
}
#endif
