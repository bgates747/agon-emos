/* INPUT-001 development service. No callbacks into application memory, no SD
 * ownership and no keyboard synthesis. READ copies resident input without
 * waiting; REQUEST uses the existing bounded foreground UART serializer and
 * never waits for a reply. CI shares the existing F7 Extender control envelope.
 * This candidate is not a production API or a new ordinary VDU route. */
#include <string.h>
#include "ff.h"
#include "emos_controller.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"
#include "emos_controller_wire.h"

static volatile BYTE active,waiting,ready,have_cache,last_rx,last_request;
static UINT32 token,acknowledged;
static UINT16 sequence;
static BYTE cache[CONTROLLER_RESULT_SIZE],request[19];
void emos_controller_reset(void){active=waiting=ready=have_cache=0;}
static BYTE range(UINT24 p,UINT24 n){return p>=0x040000&&p<0x0b0000&&n<=0x0b0000-p;}
static BYTE expired(BYTE since){return (BYTE)(emos_keyboard_clock()-since)>=30;}
void emos_controller_tick(void){
    /* Existing VBlank owner enforces deadlines even when the foreground is
     * busy, before the low-byte clock can wrap and make an old reply look new. */
    if(active&&((waiting&&expired(last_request))||(have_cache&&expired(last_rx))))emos_controller_reset();
}
static void stale_sample(BYTE*p){
    p[10]&=(BYTE)~2;p[11]=0;memset(p+34,0,12);memset(p+48,0,48);
}
static BYTE healthy(void){return uart1_keyboard_owned==1&&emos_key_source==EMOS_KEY_EXTENDER&&!emos_key_faulted;}
static BYTE send_request(BYTE operation){
    BYTE *p=request+3,result;
    if(sequence==65535){emos_controller_reset();return EMOS_UNAVAILABLE;}
    ++sequence;memset(request,0,sizeof(request));request[0]=23;request[2]=0xf7;
    p[0]='C';p[1]='I';p[2]=1;p[3]=operation;
    controller_put32(p+4,token);controller_put32(p+8,acknowledged);controller_put16(p+12,sequence);
    controller_put16(p+14,controller_crc(p,14));
    waiting=operation==1;last_request=emos_keyboard_clock();
    result=emos_keyboard_send(request,sizeof(request));
    if(result!=EMOS_KEY_OK){emos_controller_reset();return FR_TIMEOUT;}
    return FR_OK;
}
void emos_controller_packet(const BYTE*p,BYTE length){
    BYTE i;UINT24 age,interval;
    if(waiting&&expired(last_request)){emos_controller_reset();return;}
    if(!active||!waiting||ready||length!=CONTROLLER_RESULT_SIZE||memcmp(p,"CIS1",4)||
       controller_u32(p+4)!=token||controller_u16(p+8)!=sequence||p[10]>3||p[10]==2||
       p[11]>CONTROLLER_BATCH||p[46]||p[47]||
       controller_crc(p,96)!=controller_u16(p+96))return;
    if((controller_u16(p+34)&0x8000)||(controller_u16(p+34)&0x1800)==0x1800||
       (controller_u16(p+34)&0x6000)==0x6000)return;
    for(i=0;i<p[11];++i){
        const BYTE*e=p+48+i*6;
        if(!controller_u32(e)||e[4]>=15||e[5]>1||
           (i&&controller_u32(e)!=controller_u32(e-6)+1))return;
    }
    if(have_cache&&memcmp(cache+12,p+12,16))acknowledged=0;
    memcpy(cache,p,sizeof(cache));last_rx=emos_keyboard_clock();
    /* Request round-trip is a conservative bound on return transit. Include
     * one whole two-unit tick phase; never reset stale source age to zero at RX. */
    /* Native-width shifts/add avoid the unreviewed16-bit __smulu archive
     * helper selected by pinned AgonDev for short-width multiplication. */
    interval=(UINT24)(BYTE)(last_rx-last_request)+2;
    age=(UINT24)controller_u16(p+32)+(interval<<3)+interval;
    if(age>65535)age=65535;
    controller_put16(cache+32,age);
    if((cache[10]&2)&&age>=250)stale_sample(cache);
    controller_put16(cache+96,controller_crc(cache,96));
    waiting=0;ready=have_cache=1;
}
UINT24 emos_controller_gateway(t_emosGatewayRequest*r){
    UINT24 in,n,out,capacity;BYTE operation,irq,result=FR_OK;
    if(!range((UINT24)r,sizeof(*r))||!emos_application_context())return EMOS_UNSAFE_CALLER;
    in=emos_read24(r->input);n=emos_read24(r->inputLength);
    out=emos_read24(r->output);capacity=emos_read24(r->outputCapacity);
    memset(r->outputLength,0,3);
    if(n!=1||!range(in,n))return FR_INVALID_PARAMETER;
    operation=*(BYTE*)in;
    if(operation>3||(operation==1?(capacity!=CONTROLLER_RESULT_SIZE||!range(out,capacity)):(out||capacity)))return FR_INVALID_PARAMETER;
    if(emos_get_mode()!=EMOS_MODE_LEGACY&&emos_get_mode()!=EMOS_MODE_EXCLUSIVE_COMPAT)return EMOS_UNAVAILABLE;
    irq=emos_keyboard_lock();if(!irq){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    if(operation==3){
        BYTE was_active=active;active=ready=have_cache=waiting=0;
        emos_keyboard_unlock(irq);
        return was_active&&healthy()?send_request(2):FR_OK;
    }
    if(!healthy()){emos_controller_reset();emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;}
    if(operation==0){
        if(active)result=EMOS_BUSY;
        else if(token==0xffffffffUL)result=EMOS_UNAVAILABLE;
        else {++token;active=1;ready=have_cache=waiting=0;sequence=0;acknowledged=0;}
        emos_keyboard_unlock(irq);return result;
    }
    if(!active||(waiting&&expired(last_request))){emos_controller_reset();emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;}
    if(operation==1){
        if(!have_cache)result=EMOS_BUSY;
        else if(expired(last_rx)){emos_controller_reset();result=EMOS_UNAVAILABLE;}
        else {
            memcpy((BYTE*)out,cache,sizeof(cache));r->outputLength[0]=sizeof(cache);
            if((cache[10]&2)&&controller_u16(cache+32)+
               ((UINT24)(BYTE)(emos_keyboard_clock()-last_rx)+2)*9>=250)stale_sample((BYTE*)out);
            if(ready&&((BYTE*)out)[11])acknowledged=controller_u32(cache+48+(cache[11]-1)*6);
            controller_put16((BYTE*)out+96,controller_crc((BYTE*)out,96));
            ready=0;cache[11]=0;memset(cache+48,0,48);
            controller_put16(cache+96,controller_crc(cache,96));
        }
        emos_keyboard_unlock(irq);return result;
    }
    if(waiting||ready){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    emos_keyboard_unlock(irq);return send_request(1);
}
