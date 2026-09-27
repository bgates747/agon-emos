#include <agon/mos.h>
#include <string.h>
#include "sdapp.h"
extern uint24_t emos_gateway_call(uint8_t *request);
static uint8_t gateway[66],input[241];
static void put24(uint8_t *p,uint24_t n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
unsigned sdapp_link(unsigned op,const uint8_t *record,unsigned n,uint8_t *out,unsigned cap,unsigned *got){
    if(n>240)return 19;
    input[0]=op;if(n)memcpy(input+1,record,n);
    memset(gateway,0,sizeof gateway);gateway[0]=66;gateway[2]=1;
    gateway[4]=3;gateway[5]=6;gateway[6]=2;
    memcpy(gateway+25,"ext",3);memcpy(gateway+41,"sdlink",6);
    put24(gateway+10,(uint24_t)input);put24(gateway+13,n+1);
    if(cap){put24(gateway+16,(uint24_t)out);put24(gateway+19,cap);}
    unsigned r=emos_gateway_call(gateway);*got=gateway[22]|((unsigned)gateway[23]<<8);
    return r;
}
uint32_t sdapp_clock(void){return getsysvar_time();}
