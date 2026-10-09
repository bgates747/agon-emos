#define _GNU_SOURCE
#include <sys/mman.h>
#include <string.h>
#include "emos_uart_flow.h"
#include <assert.h>
#include <stdio.h>
#include "uart.h"
#include "../projects/uartflow/src/flow.h"
volatile BYTE serialFlags,uart1_keyboard_owned;
static BYTE irq=1;
BYTE emos_keyboard_lock(void){BYTE old=irq;irq=0;return old;}
void emos_keyboard_unlock(BYTE v){irq=v;}
UINT24 emos_read24(const BYTE *p){return p[0]|((UINT24)p[1]<<8)|((UINT24)p[2]<<16);}
static void put24(BYTE *p,UINT24 v){p[0]=v;p[1]=v>>8;p[2]=v>>16;}
static t_emosGatewayRequest *r=(void *)0xb0000;
static BYTE *input=(void *)0xb0100,*output=(void *)0xb0110;
static BYTE service(BYTE op,BYTE arg){
 input[0]=op;input[1]=arg;
 assert(emos_uartdiag_gateway(r)==0);assert(r->outputLength[0]==2);
 return output[0];
}
static unsigned mode, phase, ticks, phase_at, sent, received, closes;
static unsigned owns_rts, ready_calls;
BYTE emos_uart_flow_clock(void) { if (mode != 9) ticks += 2; return (BYTE)ticks; }
BYTE open_UART1(UART *s) {
    assert(s->baudRate == (mode==16 ? 115200u : 1152000u));
    assert(s->flowControl == (mode==16 ? 0 : FCTL_HW) && !s->interrupts);
    assert(!(serialFlags & 0x10)); serialFlags |= 0x30; return UART_ERR_NONE;
}
void close_UART1(void) { ++closes; serialFlags &= 0x0F; owns_rts = 0; }
BYTE uart1_claim_rts(void) { if (mode == 11) return UART_POLL_UNAVAILABLE; owns_rts = 1; return UART_POLL_READY; }
BYTE uart1_receive_ready(BYTE ready) {
    assert(owns_rts);
    if (mode == 12 && ready_calls == 2) return UART_POLL_UNAVAILABLE;
    ++ready_calls; ++phase; phase_at = ticks;
    assert(ready == (phase & 1)); return UART_POLL_READY;
}
BYTE uart1_try_put(BYTE value) {
    static const char request[] = "FLOW\r\n";
    if (phase == 1) {
        if (mode == 1 || (mode != 2 && ticks - phase_at < 120)) return UART_POLL_BLOCKED;
        if (mode == 3) return UART_POLL_ERROR;
        assert(sent < sizeof(request)-1 && value == (BYTE)request[sent]);
        ++sent; return UART_POLL_READY;
    }
    assert(phase == 3 && value == 0xA5);
    return mode == 8 ? UART_POLL_READY : UART_POLL_BLOCKED;
}
BYTE uart1_try_get(BYTE *value) {
    static const char ack[] = "FLOWACK\r\n";
    if ((phase == 2 && mode == 4) || (phase == 4 && mode == 13)) {
        *value = '!'; return UART_POLL_READY;
    }
    if (mode == 14) return UART_POLL_ERROR;
    if (phase == 3) {
        if (mode == 6 && received == 3) return UART_POLL_EMPTY;
        if (received < sizeof(ack)-1) {
            *value = mode == 5 ? 'X' : (BYTE)ack[received++]; return UART_POLL_READY;
        }
        if (mode == 7) { *value = '!'; return UART_POLL_READY; }
    }
    return UART_POLL_EMPTY;
}
BYTE flow_open(void){return service(0,0)==UART_POLL_READY;}
BYTE flow_get(BYTE *v){BYTE s=service(1,0);if(s==UART_POLL_READY)*v=output[1];return s;}
BYTE flow_put(BYTE v){return service(2,v);}
BYTE flow_ready(BYTE v){return service(3,v);}
void flow_close(void){(void)service(4,0);}
int main(void) {
 assert(mmap((void *)0xb0000,0x8000,PROT_READ|PROT_WRITE,
  MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==(void *)0xb0000);
 put24(r->input,0xb0100);put24(r->inputLength,2);
 put24(r->output,0xb0110);put24(r->outputCapacity,2);

    for (mode=0; mode<15; ++mode) {
        phase=0; ticks=250; phase_at=250; sent=received=closes=ready_calls=owns_rts=0;
        serialFlags= mode == 10 ? 0x10 : 0;
        int pass=emos_uart_flow();
        assert(pass == (mode == 0));
        assert(closes == (mode == 10 ? 0 : 1));
        assert(!owns_rts);
        assert(serialFlags == (mode == 10 ? 0x10 : 0));
    }
    puts("15 UART flow command scenarios passed");
    mode=0;serialFlags=0;closes=0;irq=1;
    input[0]=0;input[1]=0;
    const UINT24 invalid[]={0,0xaffff,0xb7fff,0xb8000,0xffffff};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i){
      put24(r->input,invalid[i]);assert(emos_uartdiag_gateway(r)==19);
      put24(r->input,0xb0100);put24(r->output,invalid[i]);assert(emos_uartdiag_gateway(r)==19);
      put24(r->output,0xb0110);assert(!serialFlags && !closes);
    }
    put24(r->inputLength,1);assert(emos_uartdiag_gateway(r)==19);put24(r->inputLength,2);
    put24(r->outputCapacity,3);assert(emos_uartdiag_gateway(r)==19);put24(r->outputCapacity,2);
    input[0]=6;assert(emos_uartdiag_gateway(r)==19);input[0]=0;input[1]=1;
    assert(emos_uartdiag_gateway(r)==19);input[1]=0;
    irq=0;assert(emos_uartdiag_gateway(r)==31 && !irq && !serialFlags);irq=1;
    uart1_keyboard_owned=1;assert(!flow_open() && !serialFlags);uart1_keyboard_owned=0;
    assert(service(4,0)==UART_POLL_UNAVAILABLE && !closes);
    assert(flow_open());assert(!flow_open() && !closes);
    emos_uartdiag_reset();assert(closes==1 && !serialFlags && !owns_rts);
    emos_uartdiag_reset();assert(closes==1);
    assert(service(1,0)==UART_POLL_UNAVAILABLE);
    mode=16;serialFlags=0;closes=owns_rts=ready_calls=0;
    assert(service(5,0)==UART_POLL_READY && serialFlags && !owns_rts);
    assert(service(0,0)==UART_POLL_UNAVAILABLE && !closes && !owns_rts);
    assert(service(5,0)==UART_POLL_UNAVAILABLE && !closes);
    assert(service(3,1)==UART_POLL_UNAVAILABLE && !ready_calls && !owns_rts);
    assert(service(3,0)==UART_POLL_UNAVAILABLE && !ready_calls);
    emos_uartdiag_reset();assert(closes==1 && !serialFlags && !owns_rts);
    input[0]=5;input[1]=1;assert(emos_uartdiag_gateway(r)==19 && closes==1);
    puts("Slow diagnostic lease, fixed configuration, RTS refusal and exit cleanup passed");
    puts("Diagnostic service bounds, IRQ/owner refusal and abandoned-lease cleanup passed");
    return 0;
}
