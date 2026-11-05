/* INTEG-019 offload: EMOS admits the private carrier and owns UART1 only.
 * Application code owns reply storage, correlation, deadlines and semantics.
 * Input is caller-owned immutable memory for this synchronous call; arbitrary
 * ISR mutation violates that contract. No new resident app pointer is retained.
 * Replies borrow the existing MOS callback for the duration of this ISR only. */
#include <string.h>
#include "ff.h"
#include "emos_net.h"
#include "emos_net_wire.h"
#include "emos_sdlink.h"
#include "emos_keyboard.h"
#include "uart.h"
extern UINT24 user_kbvector;
static BYTE admitted;
static UINT24 token;
static BYTE range(UINT24 p,UINT24 n){return p>=0x040000&&p<0x0b0000&&n<=0x0b0000-p;}
void emos_net_reset(void){admitted=0;}
void emos_net_packet(const BYTE*p,BYTE n){
    /* Fixed frame bounds make the app's NM classifier/length access safe.
     * Never publish this completion as a physical key or ordinary VDU effect. */
    if(admitted==2&&n>=AGON_NET_HEADER&&n<=AGON_NET_BODY&&
       p[12]==n-AGON_NET_HEADER&&!memcmp(p,"NM\001",3))emos_reply_callback(p);
}
UINT24 emos_net_gateway(t_emosGatewayRequest*r){
    UINT24 in,n,out,cap;BYTE op,irq,result;
    if(!range((UINT24)r,sizeof(*r))||!emos_application_context())return EMOS_UNSAFE_CALLER;
    in=emos_read24(r->input);n=emos_read24(r->inputLength);
    out=emos_read24(r->output);cap=emos_read24(r->outputCapacity);
    memset(r->outputLength,0,3);
    if(!n||!range(in,n))return FR_INVALID_PARAMETER;
    op=*(BYTE*)in;
    /* TAKE is application-local now; the former resident operation1 refuses. */
    if((op!=0&&op!=2&&op!=3)||(op==2?(n<5+AGON_NET_HEADER||n>AGON_NET_INPUT):n!=1)||
       (op==0?(cap!=7||!range(out,7)):(out||cap)))return FR_INVALID_PARAMETER;
    irq=emos_keyboard_lock();if(!irq){emos_keyboard_unlock(irq);return EMOS_BUSY;}
    if(op==3){admitted=0;emos_keyboard_unlock(irq);return FR_OK;}
    if(uart1_keyboard_owned!=1||emos_key_source!=EMOS_KEY_EXTENDER||emos_key_faulted||
       (emos_get_mode()!=EMOS_MODE_LEGACY&&emos_get_mode()!=EMOS_MODE_EXCLUSIVE_COMPAT)){
        admitted=0;emos_keyboard_unlock(irq);return EMOS_UNAVAILABLE;
    }
    if(op==0){
        if(admitted)result=EMOS_BUSY;
        else if(token==(UINT24)0xffffff)result=EMOS_UNAVAILABLE;
        else{
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
    admitted=2;emos_keyboard_unlock(irq);
    result=emos_keyboard_send((BYTE*)in+1,n-1);
    if(result!=EMOS_KEY_OK){admitted=0;return FR_TIMEOUT;}
    return FR_OK;
}
