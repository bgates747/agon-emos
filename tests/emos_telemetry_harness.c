#define _GNU_SOURCE
#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "emos_telemetry.h"
#include "emos_keyboard.h"
volatile BYTE uart1_keyboard_owned=1,emos_key_source=EMOS_KEY_EXTENDER,emos_key_faulted;
static BYTE irq=1,mode=EMOS_MODE_LEGACY,tx_result;
static BYTE sent[140];static UINT16 sent_length;
BYTE emos_keyboard_lock(void){BYTE old=irq;irq=0;return old;}
void emos_keyboard_unlock(BYTE enabled){irq=enabled;}
BYTE emos_get_mode(void){return mode;}
BYTE emos_keyboard_queue_telemetry(const BYTE *data){assert(irq);memcpy(sent,data,140);sent_length=140;return tx_result;}
UINT24 emos_read24(const BYTE *p){return p[0]|((UINT24)p[1]<<8)|((UINT24)p[2]<<16);}
static void put24(BYTE *p,UINT24 n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
int main(void){
 void *ram=mmap((void*)0x40000,0x70000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
 t_emosGatewayRequest *r=(void*)0x40000;BYTE *input=(void*)0x41000;
 assert(ram==(void*)0x40000);put24(r->input,0x41000);put24(r->inputLength,1);
 assert(emos_telemetry_gateway((void*)0x3ffff)==19);
 assert(emos_telemetry_gateway((void*)0xaffff)==19);
 mode=1;assert(emos_telemetry_gateway(r)==35);mode=0;
 irq=0;assert(emos_telemetry_gateway(r)==31);irq=1;
 assert(emos_telemetry_gateway(r)==0);
 input[0]=1;memset(input+1,0x81,140);put24(r->inputLength,141);
 assert(emos_telemetry_gateway(r)==0 && sent_length==140);
 assert(!memcmp(sent,input+1,140));
 tx_result=1;assert(emos_telemetry_gateway(r)==31);tx_result=0;
 put24(r->inputLength,142);assert(emos_telemetry_gateway(r)==19);put24(r->inputLength,141);
 put24(r->input,0xafff0);assert(emos_telemetry_gateway(r)==19);put24(r->input,0x41000);
 put24(r->output,0x42000);assert(emos_telemetry_gateway(r)==19);put24(r->output,0);
 emos_telemetry_reset();assert(emos_telemetry_gateway(r)==35);
 input[0]=0;put24(r->inputLength,1);assert(emos_telemetry_gateway(r)==0);
 input[0]=2;assert(emos_telemetry_gateway(r)==0);
 input[0]=1;put24(r->inputLength,141);assert(emos_telemetry_gateway(r)==35);
 assert(irq);munmap(ram,0x70000);puts("telemetry gateway bounds, ownership, busy and lifecycle passed");
}
