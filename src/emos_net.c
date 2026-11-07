/* INTEG-019 offload: EMOS admits the private carrier and owns UART1 only.
 * Application code owns reply storage, correlation, deadlines and semantics.
 * Input is caller-owned immutable memory for this synchronous call; arbitrary
 * ISR mutation violates that contract. Optional INTEG-024 retains only an app
 * service hook; source/count/engine state remain in application RAM.
 * Replies borrow the existing MOS callback for the duration of this ISR only. */
#include <string.h>
#include "ff.h"
#include "emos_net.h"
#include "emos_net_wire.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"
#ifdef EMOS_UART1_APP
#include "emos_neta.h"
#endif
extern UINT24 user_kbvector;
static BYTE admitted;
static UINT24 token;
static BYTE range(UINT24 p,UINT24 n){return p>=0x040000&&p<0x0b0000&&n<=0x0b0000-p;}
void emos_net_reset(void){
#ifdef EMOS_UART1_APP
    emos_neta_abort();
#endif
    admitted=0;
}
void emos_net_packet(const BYTE*p,BYTE n){
    /* Fixed frame bounds make the app's NM classifier/length access safe.
     * Never publish this completion as a physical key or ordinary VDU effect. */
    if(admitted==2&&n>=AGON_NET_HEADER&&n<=AGON_NET_BODY&&
       p[12]==n-AGON_NET_HEADER&&!memcmp(p,"NM\001",3))emos_reply_callback(p);
}
static UINT24 net_gateway(t_emosGatewayRequest*r,BYTE async){
    UINT24 in,n,out,cap;BYTE op,irq,result;
    if(!range((UINT24)r,sizeof(*r))||!emos_application_context())return EMOS_UNSAFE_CALLER;
    in=emos_read24(r->input);n=emos_read24(r->inputLength);
    out=emos_read24(r->output);cap=emos_read24(r->outputCapacity);
    memset(r->outputLength,0,3);
    if(!n||!range(in,n))return FR_INVALID_PARAMETER;
    op=*(BYTE*)in;
    /* TAKE is application-local now; the former resident operation1 refuses. */
    if((op!=0&&op!=2&&op!=3&&!(async&&op==4))||(op==2?(n<5+AGON_NET_HEADER||n>AGON_NET_INPUT):n!=(op==0&&async?4:1))||
       (op==0?(cap!=7||!range(out,7)):(out||cap)))return FR_INVALID_PARAMETER;
    irq=emos_keyboard_lock();if(!irq){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    #ifdef EMOS_UART1_APP
    /* The two provider identities never share one admitted transaction. */
    static BYTE async_owner;
    if(admitted&&async!=async_owner){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    if(async&&op==4){result=emos_neta_progress();emos_keyboard_unlock(irq);return result;}
#endif
    if(op==3){
#ifdef EMOS_UART1_APP
        if(async){result=emos_neta_close();if(result){emos_keyboard_unlock(irq);return result;}}
#endif
        admitted=0;emos_keyboard_unlock(irq);return FR_OK;
    }
    if(uart1_keyboard_owned!=1||emos_key_source!=EMOS_KEY_EXTENDER||emos_key_faulted||
       (emos_get_mode()!=EMOS_MODE_LEGACY&&emos_get_mode()!=EMOS_MODE_EXCLUSIVE_COMPAT)){
        if(!async)admitted=0;emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;
    }
    if(async&&emos_get_mode()!=EMOS_MODE_LEGACY){emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;}
    if(op==0){
        if(admitted)result=EMOS_BUSY;
        else if(token==(UINT24)0xffffff)result=EMOS_UNAVAILABLE;
        else{
            #ifdef EMOS_UART1_APP
            if(async){result=emos_neta_open(emos_read24((BYTE*)in+1));if(result){emos_keyboard_unlock(irq);return result;}}
            async_owner=async;
#endif
            ++token;memcpy((BYTE*)out,&token,3);((BYTE*)out)[3]=0;
            memcpy((BYTE*)out+4,&user_kbvector,3);r->outputLength[0]=7;
            admitted=1;result=FR_OK;
        }
        emos_keyboard_unlock(irq);return result;
    }
    if(!admitted){emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;}
    /* Endpoint framing protects EMOS routing; NM contents belong to app/P4. */
    if(memcmp((BYTE*)in+1,"\027\0\364",3)||((BYTE*)in)[4]!=n-5){
        emos_keyboard_unlock(irq);return FR_INVALID_PARAMETER;
    }
    #ifdef EMOS_UART1_APP
    if(async){
        if(!emos_application_span(in+1,n-1)){emos_keyboard_unlock(irq);return FR_INVALID_PARAMETER;}
        admitted=2;result=emos_neta_start();emos_keyboard_unlock(irq);return result;
    }
#endif
    admitted=2;emos_keyboard_unlock(irq);
    result=emos_keyboard_send((BYTE*)in+1,n-1);
    if(result!=EMOS_KEY_OK){admitted=0;return FR_TIMEOUT;}
    return FR_OK;
}

UINT24 emos_net_gateway(t_emosGatewayRequest*r){return net_gateway(r,0);}
#ifdef EMOS_UART1_APP
UINT24 emos_neta_gateway(t_emosGatewayRequest*r){return net_gateway(r,1);}
#endif
