/* INTEG-019: fixed ext.net admission/copy/correlation only. The application
 * encodes and interprets messages; P4 owns sockets. Reuses the existing EMOS
 * serializer and receive owner, not SD/controller leases. No CRC or app ISR.
 * Native Eirene worktree, not generated GNU-as/translated source. */
#include <string.h>
#include "ff.h"
#include "emos_net.h"
#include "emos_net_wire.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"
enum { CLOSED, IDLE, WAITING, READY };
static volatile BYTE state, at, length;
static UINT24 token;
static BYTE cache[AGON_NET_BODY];
static BYTE range(UINT24 p,UINT24 n){return p>=0x040000&&p<0x0b0000&&n<=0x0b0000-p;}
static BYTE healthy(void){return uart1_keyboard_owned==1&&emos_key_source==EMOS_KEY_EXTENDER&&!emos_key_faulted;}
static BYTE expired(void){return state>=WAITING&&(BYTE)(emos_keyboard_clock()-at)>=30;}
void emos_net_reset(void){state=CLOSED;}
void emos_net_tick(void){if(expired())state=CLOSED;}
void emos_net_packet(const BYTE*p,BYTE n){
    if(expired()){state=CLOSED;return;}
    if(state!=WAITING||n<AGON_NET_HEADER||n>sizeof(cache)||
       memcmp(p+4,cache+4,8)||p[3]!=(BYTE)(cache[3]|0x80))return;
    memcpy(cache,p,n);length=n;at=emos_keyboard_clock();state=READY;
}
UINT24 emos_net_gateway(t_emosGatewayRequest*r){
    UINT24 in,n,out,cap;BYTE op,irq,result;
    BYTE packet[AGON_NET_INPUT-1];
    if(!range((UINT24)r,sizeof(*r))||!emos_application_context())return EMOS_UNSAFE_CALLER;
    in=emos_read24(r->input);n=emos_read24(r->inputLength);
    out=emos_read24(r->output);cap=emos_read24(r->outputCapacity);
    memset(r->outputLength,0,3);
    if(!n||!range(in,n))return FR_INVALID_PARAMETER;
    op=*(BYTE*)in;
    if(op>3||(op==2?(n<5+AGON_NET_HEADER||n>AGON_NET_INPUT):n!=1)||
       (op==0?(cap!=4||!range(out,4)):
        op==1?(cap!=AGON_NET_BODY||!range(out,cap)):(out||cap)))return FR_INVALID_PARAMETER;
    irq=emos_keyboard_lock();if(!irq){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    if(op==3){state=CLOSED;emos_keyboard_unlock(irq);return FR_OK;}
    if(op==2){emos_keyboard_unlock(irq);memcpy(packet,(BYTE*)in+1,n-1);irq=emos_keyboard_lock();}
    if(!healthy()||(emos_get_mode()!=EMOS_MODE_LEGACY&&emos_get_mode()!=EMOS_MODE_EXCLUSIVE_COMPAT)){
        state=CLOSED;emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;
    }
    if(op==0){
        if(state)result=EMOS_BUSY;
        else if(token==(UINT24)0xffffff)result=EMOS_UNAVAILABLE;
        else{++token;memcpy(cache+4,&token,3);cache[7]=0;state=IDLE;
             memcpy((BYTE*)out,cache+4,4);r->outputLength[0]=4;result=FR_OK;}
        emos_keyboard_unlock(irq);return result;
    }
    if(!state||expired()){state=CLOSED;emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;}
    if(op==1){
        if(state!=READY){emos_keyboard_unlock(irq);return EMOS_BUSY;}
        emos_keyboard_unlock(irq);
        /* READY bytes cannot be replaced by a second ISR response. Copy with
         * IRQs enabled, then revalidate before publishing an application result. */
        memcpy((BYTE*)out,cache,length);
        irq=emos_keyboard_lock();
        if(state!=READY||!healthy()||expired()){state=CLOSED;result=EMOS_UNAVAILABLE;}
        else{r->outputLength[0]=length;state=IDLE;result=FR_OK;}
        emos_keyboard_unlock(irq);return result;
    }
    if(state!=IDLE){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    if(memcmp(packet,"\027\0\364",3)||packet[3]!=n-5||
       memcmp(packet+4,"NM\001",3)||packet[7]<1||packet[7]>3||
       memcmp(packet+8,cache+4,4)||!(packet[12]|packet[13]|packet[14]|packet[15])||
       packet[16]!=n-5-AGON_NET_HEADER||packet[17]||
       (packet[7]==1?packet[16]!=6:packet[7]==2?packet[16]!=0:packet[16]>AGON_NET_PAYLOAD)){
        emos_keyboard_unlock(irq);return FR_INVALID_PARAMETER;
    }
    memcpy(cache,packet+4,AGON_NET_HEADER);at=emos_keyboard_clock();state=WAITING;
    emos_keyboard_unlock(irq);
    result=emos_keyboard_send(packet,n-1);
    if(result!=EMOS_KEY_OK){state=CLOSED;return FR_TIMEOUT;}
    return FR_OK;
}
