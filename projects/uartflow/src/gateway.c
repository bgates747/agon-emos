/* Fixed ext.uartdiag v1 messages; reuse the established API 0x51 binding. */
#include <agon/mos.h>
#include <string.h>
#include "flow.h"
extern uint24_t emos_gateway_call(uint8_t *request);
static uint8_t request[66],input[2],output[2];
static void put24(uint8_t *p,uint24_t n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
static BYTE call(BYTE op,BYTE arg) {
    input[0]=op;input[1]=arg;output[0]=UART_POLL_UNAVAILABLE;output[1]=0;
    memset(request,0,sizeof request);request[0]=66;request[2]=1;
    request[4]=3;request[5]=8;request[6]=2;
    memcpy(request+25,"ext",3);memcpy(request+41,"uartdiag",8);
    put24(request+10,(uint24_t)input);request[13]=2;
    put24(request+16,(uint24_t)output);request[19]=2;
    if(emos_gateway_call(request) || request[22]!=2 || request[23] || request[24])
        return UART_POLL_UNAVAILABLE;
    return output[0];
}
BYTE flow_open_slow(void){return call(5,0)==UART_POLL_READY;}
BYTE flow_open(void){return call(0,0)==UART_POLL_READY;}
BYTE flow_get(BYTE *v){BYTE r=call(1,0);if(r==UART_POLL_READY)*v=output[1];return r;}
BYTE flow_put(BYTE v){return call(2,v);}
BYTE flow_ready(BYTE v){return call(3,v);}
void flow_close(void){(void)call(4,0);}
BYTE emos_uart_flow_clock(void){return (BYTE)getsysvar_time();}
