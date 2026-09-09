#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"
#include "emos_uart_probe.h"
#include "emos_keyboard.h"
/* These legacy one-shot tests have no resident keyboard owner. Shared-session
 * effects are exercised against the real receiver in emos_keyboard_harness.c. */
volatile BYTE uart1_keyboard_owned;
BYTE emos_keyboard_text(const BYTE *p, UINT16 n) { (void)p; (void)n; return EMOS_KEY_BUSY; }

volatile BYTE serialFlags;
static unsigned mode,ticks,sent,received,closed,claimed,attempts,clock_polls;
static const BYTE *payload;
static unsigned length;
static BYTE token;
BYTE emos_uart_probe_clock(void) { if(mode!=8 && ++clock_polls%32==0) ticks+=2; return (BYTE)ticks; }
BYTE open_UART1(UART *s) {
 assert(s->baudRate==1152000 && s->flowControl==FCTL_HW && !s->interrupts);
 if(mode==9)return UART_ERR_FAILURE;
 serialFlags|=0x10;return UART_ERR_NONE;
}
void close_UART1(void){++closed;serialFlags=0;claimed=0;}
BYTE uart1_claim_rts(void){if(mode==10)return UART_POLL_UNAVAILABLE;claimed=1;return UART_POLL_READY;}
BYTE uart1_receive_ready(BYTE b){assert(b==1 && claimed);return mode==11?UART_POLL_UNAVAILABLE:UART_POLL_READY;}
BYTE uart1_try_put(BYTE b){
 const BYTE suffix[]={23,0,0xCA,23,0,0x80,token};
 if(mode==6 || (mode==13 && ++attempts<6))return UART_POLL_BLOCKED;
 if(mode==7)return UART_POLL_ERROR;
 assert(sent<length+7 && b==(sent<length?payload[sent]:suffix[sent-length]));
 ++sent;return UART_POLL_READY;
}
BYTE uart1_try_get(BYTE *b){
 const BYTE ack[]={0x80,1,token};assert(sent==length+7);
 if(mode==1 || (mode==2 && received==2))return UART_POLL_EMPTY;
 if(mode==5)return UART_POLL_ERROR;
 if(received<3){*b=ack[received++];if(mode==3)*b=0x81;return UART_POLL_READY;}
 if(mode==4){*b=token;return UART_POLL_READY;}
 return UART_POLL_EMPTY;
}
static void reset(void) {
 ticks=250;sent=received=closed=claimed=attempts=clock_polls=0;serialFlags=mode==12?0x10:0;
}
int main(void){
 static const BYTE legacy[]="\x0c\x1f\x02\x02" "EMOS TO EDP: UART TEXT\r\n";
 static const BYTE alternate[]="\x0c" "A different SD sample\r\n42\r\n";
 BYTE maximum[EMOS_TEXT_LIMIT]; memset(maximum,'X',sizeof(maximum));
 for(unsigned variant=0;variant<3;++variant){
  payload=variant==0?legacy:variant==1?alternate:maximum;
  length=variant==0?sizeof(legacy)-1:variant==1?sizeof(alternate)-1:sizeof(maximum);
  token=variant==0?0xA6:0xA7;
  for(mode=0;mode<14;++mode){
   reset();
   assert((variant==0?emos_visible_text():emos_text_probe(payload,length))==(mode==0 || mode==13));
   assert(closed==(mode==9||mode==12?0:1));assert(!claimed);assert(serialFlags==(mode==12?0x10:0));
  }
 }
 mode=0;reset();
 assert(!emos_text_probe(NULL,1));assert(!emos_text_probe(maximum,0));
 assert(!emos_text_probe(maximum,EMOS_TEXT_LIMIT+1));
 const BYTE bad[][3]={{22,3,0},{23,0,0x80},{0,0,0},{31,2,0}};
 for(unsigned i=0;i<4;++i)assert(!emos_text_probe(bad[i],i==3?2:3));
 assert(!closed && !sent && !serialFlags);
 puts("42 visible text transport scenarios and invalid-input checks passed");
}
