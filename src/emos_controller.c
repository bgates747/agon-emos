/* INPUT-005 / INTEG-018: fixed controller transport only. The application
 * library owns CI encoding, controller validation, source/edge bookkeeping and
 * freshness policy. EMOS owns UART1, one outstanding request, bounded storage
 * and safe foreground delivery. No app pointer/ISR callback, SD lease or CRC.
 * Gateway ops4..7 deliberately refuse the old resident-semantic ops0..3. */
#include <string.h>
#include "ff.h"
#include "emos_controller.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"
/* This transport uses only wire constants; canonical static decode helpers
 * remain shared with P4/app consumers and intentionally unused here. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "emos_controller_wire.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* A single state rules out overlapping TX/READ. READY cache bytes are immutable
 * until a later foreground SEND; tick/fault cleanup changes only state. */
enum { CLOSED, IDLE, WAITING, READY };
static volatile BYTE state,requested_at,received_at;
static UINT24 token;
static BYTE cache[CONTROLLER_RESULT_SIZE];
void emos_controller_reset(void){state=CLOSED;}
static BYTE range(UINT24 p,UINT24 n){return p>=0x040000&&p<0x0b0000&&n<=0x0b0000-p;}
static BYTE healthy(void){return uart1_keyboard_owned==1&&emos_key_source==EMOS_KEY_EXTENDER&&!emos_key_faulted;}
static BYTE expired(void){
    return state>=WAITING&&(BYTE)(emos_keyboard_clock()-(state==WAITING?requested_at:received_at))>=30;
}
void emos_controller_tick(void){if(expired())state=CLOSED;}
void emos_controller_packet(const BYTE*p,BYTE length){
    if(expired()){state=CLOSED;return;}
    /* Correlation lives at the same offsets in cache and response. Other
     * bytes are opaque here; application validation precedes its edge ACK. */
    if(state!=WAITING||length!=sizeof(cache)||memcmp(p+4,cache+4,6))return;
    memcpy(cache,p,sizeof(cache));received_at=emos_keyboard_clock();state=READY;
}
UINT24 emos_controller_gateway(t_emosGatewayRequest*r){
    UINT24 in,n,out,capacity;BYTE operation,irq,result;
    BYTE packet[19];
    if(!range((UINT24)r,sizeof(*r))||!emos_application_context())return EMOS_UNSAFE_CALLER;
    in=emos_read24(r->input);n=emos_read24(r->inputLength);
    out=emos_read24(r->output);capacity=emos_read24(r->outputCapacity);
    memset(r->outputLength,0,3);
    if(!n||!range(in,n))return FR_INVALID_PARAMETER;
    operation=*(BYTE*)in;
    if(operation<CONTROLLER_GATEWAY_OPEN||operation>CONTROLLER_GATEWAY_CLOSE||
       n!=(operation==CONTROLLER_GATEWAY_SEND?CONTROLLER_GATEWAY_SEND_SIZE:1)||
       (operation==CONTROLLER_GATEWAY_OPEN?(capacity!=4||!range(out,4)):
        operation==CONTROLLER_GATEWAY_RECEIVE?(capacity!=CONTROLLER_GATEWAY_RESULT_SIZE||!range(out,capacity)):
        (out||capacity)))return FR_INVALID_PARAMETER;
    irq=emos_keyboard_lock();if(!irq){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    /* Local release must succeed after mode changes, expiry or UART failure. */
    if(operation==CONTROLLER_GATEWAY_CLOSE){state=CLOSED;emos_keyboard_unlock(irq);return FR_OK;}
    if(operation==CONTROLLER_GATEWAY_SEND){
        emos_keyboard_unlock(irq);
        /* Validate an owned TX snapshot, never stream mutable application bytes
         * after checking its endpoint. The stack copy lives through the bounded
         * serializer call only; no static TX buffer or app pointer is retained. */
        memcpy(packet,(BYTE*)in+1,sizeof(packet));
        irq=emos_keyboard_lock();
    }
    if(!healthy()||(emos_get_mode()!=EMOS_MODE_LEGACY&&emos_get_mode()!=EMOS_MODE_EXCLUSIVE_COMPAT)){
        state=CLOSED;emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;
    }
    if(operation==CONTROLLER_GATEWAY_OPEN){
        if(state)result=EMOS_BUSY;
        else if(token==(UINT24)0xffffff)result=EMOS_UNAVAILABLE;
        else{
            ++token;memcpy(cache+4,&token,3);cache[7]=0;state=IDLE;
            memcpy((BYTE*)out,cache+4,4);r->outputLength[0]=4;result=FR_OK;
        }
        emos_keyboard_unlock(irq);return result;
    }
    if(!state||expired()){state=CLOSED;emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;}
    if(operation==CONTROLLER_GATEWAY_RECEIVE){
        if(state!=READY){emos_keyboard_unlock(irq);return EMOS_BUSY;}
        emos_keyboard_unlock(irq);
        /* IN001-F02: bulk copy with IRQs enabled. No pending response can
         * overwrite READY storage; application exit/fault/tick only revoke it.
         * Read cache age after the copy and revalidate before publishing. */
        memcpy((BYTE*)out,cache,sizeof(cache));
        ((BYTE*)out)[96]=(BYTE)(received_at-requested_at);
        ((BYTE*)out)[97]=(BYTE)(emos_keyboard_clock()-received_at);
        irq=emos_keyboard_lock();
        if(state!=READY||!healthy()||expired()){state=CLOSED;result=EMOS_UNAVAILABLE;}
        else{state=IDLE;r->outputLength[0]=CONTROLLER_GATEWAY_RESULT_SIZE;result=FR_OK;}
        emos_keyboard_unlock(irq);return result;
    }
    /* Fixed endpoint only: applications cannot substitute console activation
     * or routing commands. P4 retains its own CI body/version validation. */
    if(memcmp(packet,"\027\0\367CI\002",6)||packet[6]<1||packet[6]>2||
       memcmp(packet+7,cache+4,4)||!(packet[15]|packet[16])){
        emos_keyboard_unlock(irq);return FR_INVALID_PARAMETER;
    }
    if(packet[6]==1&&state!=IDLE){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    memcpy(cache+8,packet+15,2);
    requested_at=emos_keyboard_clock();state=packet[6]==1?WAITING:IDLE;
    emos_keyboard_unlock(irq);
    result=emos_keyboard_send(packet,sizeof(packet));
    if(result!=EMOS_KEY_OK){state=CLOSED;return FR_TIMEOUT;}
    return FR_OK;
}
