#include <assert.h>
#include <stdio.h>
#include "uart.h"
#include "emos_uart_probe.h"
#include "emos_keyboard.h"
/* These legacy one-shot tests have no resident keyboard owner. Shared-session
 * effects are exercised against the real receiver in emos_keyboard_harness.c. */
volatile BYTE uart1_keyboard_owned;
BYTE emos_keyboard_text(const BYTE *p, UINT16 n) { (void)p; (void)n; return EMOS_KEY_BUSY; }

volatile BYTE serialFlags;
static unsigned mode,ticks,sent,received,closed,claimed,attempts;
BYTE emos_uart_probe_clock(void) { if(mode!=8) ticks+=2; return (BYTE)ticks; }
BYTE open_UART1(UART *s) {
 assert(s->baudRate==1152000 && s->flowControl==FCTL_HW && !s->interrupts);
 if(mode==9)return UART_ERR_FAILURE;
 serialFlags|=0x10;return UART_ERR_NONE;
}
void close_UART1(void){++closed;serialFlags=0;claimed=0;}
BYTE uart1_claim_rts(void){if(mode==10)return UART_POLL_UNAVAILABLE;claimed=1;return UART_POLL_READY;}
BYTE uart1_receive_ready(BYTE b){assert(b==1 && claimed);return mode==11?UART_POLL_UNAVAILABLE:UART_POLL_READY;}
BYTE uart1_try_put(BYTE b){
 const BYTE req[]={23,0,0x80,0xA5};
 if(mode==6 || (mode==13 && ++attempts<6))return UART_POLL_BLOCKED;
 if(mode==7)return UART_POLL_ERROR;
 assert(sent<4 && b==req[sent++]);return UART_POLL_READY;
}
BYTE uart1_try_get(BYTE *b){
 const BYTE ack[]={0x80,1,0xA5};assert(sent==4);
 if(mode==1 || (mode==2 && received==2))return UART_POLL_EMPTY;
 if(mode==5)return UART_POLL_ERROR;
 if(received<3){*b=ack[received++];if(mode==3)*b=0x81;return UART_POLL_READY;}
 if(mode==4){*b=0xA5;return UART_POLL_READY;}
 return UART_POLL_EMPTY;
}
int main(void){
 for(mode=0;mode<14;++mode){
  ticks=250;sent=received=closed=claimed=attempts=0;serialFlags=mode==12?0x10:0;
  assert(emos_general_poll()==(mode==0 || mode==13));assert(closed==(mode==9||mode==12?0:1));
  assert(!claimed);assert(serialFlags==(mode==12?0x10:0));
 }
 puts("14 General Poll command scenarios passed");
}
