/* Test the extracted algorithm against the real resident diagnostic lease.
 * Only UART leaves and clock are test doubles; no diagnostic policy is mocked.
 */
#include <sys/mman.h>
#include "emos_uart_flow.h"
static BYTE probe_irq=1;
BYTE emos_keyboard_lock(void){BYTE old=probe_irq;probe_irq=0;return old;}
void emos_keyboard_unlock(BYTE value){probe_irq=value;}
UINT24 emos_read24(const BYTE *p){return p[0]|((UINT24)p[1]<<8)|((UINT24)p[2]<<16);}
static t_emosGatewayRequest *probe_request=(void *)0xb0000;
static BYTE *probe_input=(void *)0xb0100,*probe_output=(void *)0xb0110;
static void probe_put24(BYTE *p,UINT24 n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
static void probe_init(void){
 assert(mmap((void *)0xb0000,0x8000,PROT_READ|PROT_WRITE,
 MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==(void *)0xb0000);
 probe_put24(probe_request->input,0xb0100);probe_put24(probe_request->inputLength,2);
 probe_put24(probe_request->output,0xb0110);probe_put24(probe_request->outputCapacity,2);
}
static BYTE probe_service(BYTE op,BYTE arg){
 probe_input[0]=op;probe_input[1]=arg;
 assert(emos_uartdiag_gateway(probe_request)==0);
 assert(probe_request->outputLength[0]==2);
 return probe_output[0];
}
BYTE flow_open(void){return probe_service(0,0)==UART_POLL_READY;}
BYTE flow_open_slow(void){return probe_service(5,0)==UART_POLL_READY;}
BYTE flow_get(BYTE *v){BYTE s=probe_service(1,0);if(s==UART_POLL_READY)*v=probe_output[1];return s;}
BYTE flow_put(BYTE v){return probe_service(2,v);}
BYTE flow_ready(BYTE v){return probe_service(3,v);}
void flow_close(void){(void)probe_service(4,0);}
