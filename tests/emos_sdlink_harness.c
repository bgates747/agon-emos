#define _GNU_SOURCE
#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "emos_sdlink.h"
#include "emos_keyboard.h"

volatile BYTE uart1_keyboard_owned=1, emos_key_source=EMOS_KEY_EXTENDER, emos_key_faulted;
static BYTE irq=1, mode=EMOS_MODE_LEGACY, tx_result;
static BYTE sent[244]; static UINT16 sent_length;
BYTE emos_keyboard_lock(void) { BYTE old=irq;irq=0;return old; }
void emos_keyboard_unlock(BYTE enabled) { irq=enabled; }
BYTE emos_get_mode(void) { return mode; }
BYTE emos_keyboard_send(const BYTE *data,UINT16 n) {
    assert(irq);assert(n<=sizeof(sent));memcpy(sent,data,n);sent_length=n;
    return tx_result;
}
UINT24 emos_read24(const BYTE *p) { return p[0]|((UINT24)p[1]<<8)|((UINT24)p[2]<<16); }
static void put24(BYTE *p,UINT24 n) {p[0]=n;p[1]=n>>8;p[2]=n>>16;}
int main(void) {
    void *ram=mmap((void *)0x40000,0x70000,PROT_READ|PROT_WRITE,
                  MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    t_emosGatewayRequest *r=(void *)0x40000;
    BYTE *input=(void *)0x41000,*output=(void *)0x42000,data[240];
    assert(ram==(void *)0x40000);
    memset(data,0x8D,sizeof(data));
    put24(r->input,0x41000);put24(r->inputLength,1);
    assert(emos_sdlink_gateway((void *)0x3ffff)==19);
    assert(emos_sdlink_gateway((void *)0xaffff)==19);
    mode=1;assert(emos_sdlink_gateway(r)==35);mode=0;
    emos_key_source=1;assert(emos_sdlink_gateway(r)==35);emos_key_source=2;
    irq=0;assert(emos_sdlink_gateway(r)==31);irq=1;
    assert(emos_sdlink_gateway(r)==0); /* OPEN */
    emos_sdlink_packet(data,240);
    data[0]=17;emos_sdlink_packet(data,240); /* Full mailbox cannot overwrite. */
    input[0]=1;put24(r->output,0x42000);put24(r->outputCapacity,240);
    assert(emos_sdlink_gateway(r)==0 && r->outputLength[0]==240);
    assert(output[0]==0x8D && output[239]==0x8D);
    assert(emos_sdlink_gateway(r)==0 && r->outputLength[0]==0);
    emos_sdlink_packet(data,19);
    assert(emos_sdlink_gateway(r)==0 && r->outputLength[0]==0);
    put24(r->output,0xafff0);
    assert(emos_sdlink_gateway(r)==19);put24(r->output,0x42000);
    put24(r->outputCapacity,239);assert(emos_sdlink_gateway(r)==19);
    put24(r->outputCapacity,0);put24(r->output,0);
    input[0]=2;memcpy(input+1,data,240);put24(r->inputLength,241);
    assert(emos_sdlink_gateway(r)==0 && sent_length==244);
    assert(sent[0]==23 && sent[1]==0 && sent[2]==0xF6 && sent[3]==240);
    assert(!memcmp(sent+4,data,240));
    put24(r->inputLength,242);assert(emos_sdlink_gateway(r)==19);
    put24(r->inputLength,241);tx_result=2;assert(emos_sdlink_gateway(r)==15);
    emos_sdlink_reset(); /* No admission/old message survives an application exit. */
    tx_result=0;assert(emos_sdlink_gateway(r)==35);
    input[0]=0;put24(r->inputLength,1);assert(emos_sdlink_gateway(r)==0);
    input[0]=3;assert(emos_sdlink_gateway(r)==0);
    input[0]=1;put24(r->output,0x42000);put24(r->outputCapacity,240);
    assert(emos_sdlink_gateway(r)==35);
    assert(irq);
    assert(munmap(ram,0x70000)==0);
    puts("sdlink gateway bounds, mailbox, ownership and lifecycle passed");
}
