/* INTEG-020 / NET-004 optional ext.netp. Mainboard output stays Legacy UART0.
 * The app owns NM correlation, policy and buffers. Resident state is only one
 * admitted carrier token; no app pointer survives this function.
 * Transport binding is deliberately required at link time, never UART fallback.
 */
#ifdef EMOS_NETWORK_PARALLEL
#include <string.h>
#include "ff.h"
#include "emos_netp_transport.h"
#include "emos_sdlink.h"
extern volatile BYTE emosVduBackend;
#define active emos_netp_token

static __attribute__((noinline)) BYTE range(UINT24 p,UINT24 n){return p>=0x040000&&p<0x0b0000&&n<=0x0b0000-p;}
/* The carrier retains token ownership across an unclean app return. No app
 * pointer survives; a new OPEN refuses until positive RELEASE or hardware reset. */
BYTE emos_netp_owned(void){return active!=0;}
UINT24 emos_netp_gateway(t_emosGatewayRequest*r){
    UINT24 in,n,out,cap;UINT16 length=0;BYTE op,result;
    if(!range((UINT24)r,sizeof(*r))||!emos_application_context())return EMOS_UNSAFE_CALLER;
    in=emos_read24(r->input);n=emos_read24(r->inputLength);
    out=emos_read24(r->output);cap=emos_read24(r->outputCapacity);
    memset(r->outputLength,0,3);
    if(!n||!range(in,n))return FR_INVALID_PARAMETER;
    op=*(BYTE*)in;
    if(op>3 || (op==0?n!=1:op==2?(n<19||n>147):n!=5))return FR_INVALID_PARAMETER;
    if(op==0 ? (cap!=8||!range(out,8)) : op==1 ? (!cap||cap>EMOS_NETP_MAX||!range(out,cap)) : (out||cap))
        return FR_INVALID_PARAMETER;
    /* Shared route test for OPEN and data operations; RELEASE stays available. */
    if(op!=3&&(emos_get_mode()!=EMOS_MODE_LEGACY||emosVduBackend))return EMOS_UNAVAILABLE;
    if(op==0){
        if(active)return EMOS_BUSY;
        result=emos_netp_link_open();
        if(result)return result;
        memcpy((BYTE*)out,"NP\001\001",4);memcpy((BYTE*)out+4,&active,4);
        r->outputLength[0]=8;return FR_OK;
    }
    if(!active||memcmp((BYTE*)in+1,&active,4))return EMOS_UNAVAILABLE;
    /* Cleanup remains callable after route/input loss. Never fake release. */
    if(op==3){
        return emos_netp_link_release();
    }
    if(op==2){
        result=emos_netp_link_submit((BYTE*)in+5,(UINT16)(n-5));
    }else{
        result=emos_netp_link_take((BYTE*)out,(UINT16)cap,&length);
        if((result==FR_OK&&(length<14||length>cap))||
           (result==EMOS_NETP_SMALL&&(length<=cap||length>EMOS_NETP_MAX)))result=EMOS_NETP_FAULT;
        if(!result||result==EMOS_NETP_SMALL){
            r->outputLength[0]=(BYTE)length;r->outputLength[1]=(BYTE)(length>>8);
        }
    }
    return result;
}
#endif
