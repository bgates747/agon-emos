#define _GNU_SOURCE
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "emos_controller.h"
#include "emos_controller_wire.h"
#include "emos_keyboard.h"
volatile BYTE emos_key_source=2,emos_key_faulted=0,uart1_keyboard_owned=1;
static BYTE irq=1,clock_value=50,context=1,mode=0,send_result=0;
static BYTE sent[19];static unsigned sends;
BYTE emos_keyboard_lock(void){BYTE old=irq;irq=0;return old;}
void emos_keyboard_unlock(BYTE v){irq=v;}
BYTE emos_keyboard_clock(void){return clock_value;}
BYTE emos_application_context(void){return context;}
BYTE emos_get_mode(void){return mode;}
BYTE emos_keyboard_send(const BYTE*p,UINT16 n){assert(irq&&n==19);memcpy(sent,p,n);++sends;return send_result;}
UINT24 emos_read24(const BYTE*p){return p[0]|((UINT24)p[1]<<8)|((UINT24)p[2]<<16);}
static void put24(BYTE*p,UINT24 n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
static t_emosGatewayRequest*r=(void*)0x40000;
static BYTE*input=(void*)0x40100,*output=(void*)0x40200;
static BYTE service(BYTE op){
    input[0]=op;put24(r->input,0x40100);put24(r->inputLength,1);
    put24(r->output,op==1?0x40200:0);put24(r->outputCapacity,op==1?CONTROLLER_RESULT_SIZE:0);
    return emos_controller_gateway(r);
}
static void response(BYTE*p,BYTE count){
    memset(p,0,CONTROLLER_RESULT_SIZE);memcpy(p,CONTROLLER_RESULT_MAGIC,4);memcpy(p+4,sent+7,4);memcpy(p+8,sent+15,2);
    p[10]=3;p[11]=count;controller_put32(p+12,100);controller_put32(p+20,200);
    controller_put32(p+28,1);controller_put16(p+36,32767);p[44]=255;
    if(count){controller_put32(p+48,1);p[52]=0;p[53]=1;controller_put32(p+54,2);p[58]=0;p[59]=0;}
}
int main(void){
    BYTE p[CONTROLLER_RESULT_SIZE];
    assert(mmap((void*)0x40000,0x10000,PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==(void*)0x40000);
    assert(service(0)==0&&service(0)==31);assert(service(1)==31);
    assert(service(2)==0&&sends==1&&sent[2]==0xf7&&sent[5]==CONTROLLER_CARRIER_VERSION&&!sent[17]&&!sent[18]);
    assert(service(2)==31&&sends==1);
    response(p,2);emos_controller_packet(p,sizeof(p)+2);assert(service(1)==31);
    p[3]^=1;emos_controller_packet(p,sizeof(p));assert(service(1)==31);
    p[3]^=1;emos_controller_packet(p,sizeof(p));assert(service(1)==0&&r->outputLength[0]==CONTROLLER_RESULT_SIZE&&output[11]==2);
    output[36]=0;assert(service(1)==0&&output[11]==0&&controller_u16(output+36)==32767);
    assert(output[11]==0&&!memcmp(output+48,(BYTE[48]){0},48));
    assert(service(2)==0&&controller_u32(sent+11)==2);
    response(p,0);controller_put16(p+8,1);
    emos_controller_packet(p,sizeof(p));assert(service(2)==31);
    response(p,0);controller_put32(p+20,201);
    emos_controller_packet(p,sizeof(p));assert(service(1)==0&&service(2)==0&&controller_u32(sent+11)==0);
    clock_value+=30;assert(service(1)==35&&service(2)==35);assert(service(0)==0&&service(2)==0);
    // Previous lease reply is ignored even when its query number repeats.
    p[8]=1;emos_controller_packet(p,sizeof(p));
    assert(service(1)==31);
    response(p,0);controller_put16(p+32,240);
    emos_controller_packet(p,sizeof(p));assert(service(1)==0&&!(output[10]&2));
    assert(service(3)==0&&service(0)==0&&service(2)==0);response(p,0);
    // Expired response and then a whole low-byte-clock wrap cannot revive it.
    clock_value+=30;emos_controller_tick();clock_value+=226;
    emos_controller_packet(p,sizeof(p));assert(service(1)==35);
    assert(service(0)==0);assert(service(3)==0&&sent[6]==2&&service(1)==35);
    irq=0;assert(service(0)==31&&!irq);irq=1;context=0;assert(service(0)==32);context=1;
    mode=3;assert(service(0)==35);mode=0;
    uart1_keyboard_owned=2;assert(service(0)==35);uart1_keyboard_owned=1;
    assert(service(0)==0);emos_key_faulted=1;assert(service(1)==35);emos_key_faulted=0;
    send_result=3;assert(service(0)==0&&service(2)==15&&service(1)==35);send_result=0;
    put24(r->input,0x3ffff);assert(emos_controller_gateway(r)==19);
    puts("Controller cache, tap acknowledgement, integrity, ownership and expiry checks pass");
}
