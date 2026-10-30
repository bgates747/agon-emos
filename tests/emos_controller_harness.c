#define _GNU_SOURCE
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "emos_controller.h"
#include "emos_controller_wire.h"
#include "emos_keyboard.h"
volatile BYTE emos_key_source=2,emos_key_faulted=0,uart1_keyboard_owned=1;
static BYTE irq=1,clock_value=50,context=1,mode=0,send_result=0,copy_hook;
static BYTE sent[19],owner_token[4];static unsigned sends;
static t_emosGatewayRequest*r=(void*)0x40000;
static BYTE*input=(void*)0x40100,*output=(void*)0x40200;
/* Replace only byte-copy effects so a revocation can occur at the actual
 * unlocked-copy boundary. The complete resident service remains linked. */
void *controller_test_copy(void*d,const void*s,size_t n){
    size_t i;BYTE hook=copy_hook;
    if(d==output&&n==96){assert(irq);copy_hook=0;
        if(hook==1){clock_value+=30;emos_controller_tick();}
        if(hook==2)emos_key_faulted=1;
    }
    for(i=0;i<n;++i)((BYTE*)d)[i]=((const BYTE*)s)[i];
    if(s==input+1&&n==19&&hook==3){copy_hook=0;input[4]='E';input[5]='X';}
    return d;
}
BYTE emos_keyboard_lock(void){BYTE old=irq;irq=0;return old;}
void emos_keyboard_unlock(BYTE v){irq=v;}
BYTE emos_keyboard_clock(void){return clock_value;}
BYTE emos_application_context(void){return context;}
BYTE emos_get_mode(void){return mode;}
BYTE emos_keyboard_send(const BYTE*p,UINT16 n){assert(irq&&n==19);memcpy(sent,p,n);++sends;return send_result;}
UINT24 emos_read24(const BYTE*p){return p[0]|((UINT24)p[1]<<8)|((UINT24)p[2]<<16);}
static void put24(BYTE*p,UINT24 n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
static BYTE service(BYTE op){
    input[0]=op;put24(r->input,0x40100);put24(r->inputLength,op==6?20:1);
    put24(r->output,(op==4||op==5)?0x40200:0);put24(r->outputCapacity,op==4?4:op==5?98:0);
    return emos_controller_gateway(r);
}
static void open_owner(void){assert(service(4)==0&&r->outputLength[0]==4);memcpy(owner_token,output,4);assert(controller_u32(owner_token)&&!owner_token[3]);}
static void request(BYTE op,unsigned query){
    BYTE*p=input+1;memset(p,0,19);p[0]=23;p[2]=0xf7;p[3]='C';p[4]='I';p[5]=2;p[6]=op;
    memcpy(p+7,owner_token,4);controller_put32(p+11,0);controller_put16(p+15,query);
}
static void response(BYTE*p){
    memset(p,0,96);memcpy(p,"CIS2",4);memcpy(p+4,sent+7,4);memcpy(p+8,sent+15,2);
    p[10]=3;controller_put16(p+32,240);controller_put16(p+36,32767);p[44]=255;
}
static void deliver(BYTE*p){BYTE before=irq;irq=0;emos_controller_packet(p,96);irq=before;}
int main(void){
    BYTE p[96],first_token[4];unsigned before;
    assert(mmap((void*)0x40000,0x10000,PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==(void*)0x40000);
    for(BYTE op=0;op<4;++op)assert(service(op)==19);
    assert(service(5)==35);open_owner();memcpy(first_token,owner_token,4);
    assert(service(4)==31&&service(5)==31);
    request(1,1);assert(service(6)==0&&sends==1);assert(service(6)==31&&service(5)==31);
    response(p);irq=0;emos_controller_packet(p,98);irq=1;assert(service(5)==31);
    p[4]^=1;deliver(p);assert(service(5)==31);p[4]^=1;
    p[8]^=1;deliver(p);assert(service(5)==31);p[8]^=1;
    clock_value+=4;deliver(p);assert(service(6)==31);clock_value+=2;
    assert(service(5)==0&&r->outputLength[0]==98&&!memcmp(output,p,96));
    assert(output[96]==4&&output[97]==2&&controller_u16(output+32)==240);
    assert(service(5)==31); /* One-shot: retained state belongs to application. */
    /* Raw malformed controller contents are copied; app validation owns them. */
    request(1,2);assert(service(6)==0);response(p);p[0]='?';p[10]=255;p[11]=255;p[34]=255;
    deliver(p);assert(service(5)==0&&!memcmp(output,p,96));
    /* Fixed envelope/token/query guards are essential transport admission. */
    request(1,3);input[4]='E';assert(service(6)==19);
    request(1,3);input[8]^=1;assert(service(6)==19);
    request(3,3);assert(service(6)==19);request(1,0);assert(service(6)==19);
    /* Snapshot validation is independent of callback mutation of caller input. */
    request(1,3);copy_hook=3;assert(service(6)==0&&sent[3]=='C'&&sent[4]=='I');
    response(p);deliver(p);assert(service(5)==0);
    /* Typed close cancels pending reply, then local close always revokes. */
    request(1,4);assert(service(6)==0);response(p);
    request(2,5);assert(service(6)==0&&service(5)==31);deliver(p);assert(service(5)==31);
    assert(service(7)==0);open_owner();assert(memcmp(first_token,owner_token,4));
    request(1,1);assert(service(6)==0);deliver(p);assert(service(5)==31);
    response(p);deliver(p);assert(service(5)==0);
    /* Outstanding/cache expiry and a full clock wrap cannot revive a lease. */
    request(1,2);assert(service(6)==0);response(p);clock_value+=30;emos_controller_tick();clock_value+=226;
    deliver(p);assert(service(5)==35);open_owner();request(1,1);assert(service(6)==0);response(p);deliver(p);
    clock_value+=30;emos_controller_tick();assert(service(5)==35);
    /* Revocation midway through unlocked copy must publish no success length. */
    open_owner();request(1,1);assert(service(6)==0);response(p);deliver(p);copy_hook=1;
    assert(service(5)==35&&!r->outputLength[0]);
    open_owner();request(1,1);assert(service(6)==0);response(p);deliver(p);copy_hook=2;
    assert(service(5)==35&&!r->outputLength[0]);emos_key_faulted=0;
    open_owner();request(1,1);send_result=3;assert(service(6)==15&&service(5)==35);send_result=0;
    /* CLOSE does not depend on current display route or UART health. */
    open_owner();mode=3;emos_key_faulted=1;assert(service(7)==0);mode=0;emos_key_faulted=0;
    irq=0;assert(service(4)==31&&!irq);irq=1;context=0;assert(service(4)==32);context=1;
    mode=3;assert(service(4)==35);mode=0;uart1_keyboard_owned=2;assert(service(4)==35);uart1_keyboard_owned=1;
    before=sends;input[0]=4;put24(r->input,0x3ffff);assert(emos_controller_gateway(r)==19&&sends==before);
    puts("Controller transport, raw one-shot copy, fixed endpoint, lifetime and expiry checks pass");
}
