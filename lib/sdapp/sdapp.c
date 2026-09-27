/* R05-A05: foreground application-owned checked transfers. No direct UART,
 * implicit route change, nested load, dynamic allocation or retry of a mutation.
 * The existing service engine owns mainboard FAT staging/verification. Its
 * sibling journal scheme is recoverable, not power-atomic (REMOTE-005/A05).
 * A remote peer must implement the frozen application control contract; the
 * ordinary P4 console is intentionally rejected during capability negotiation.
 */
#include <string.h>
#include "sdapp.h"
#include "service.h"
static uint8_t tx[240],rx[240],local_rx[240],prefix[28],descriptor[248],chunk[212];
static uint32_t token,sequence,file_session,file_sequence,local_sequence;
static unsigned busy,cancelled,opened,initialized,claimed,local_stage,remote_stage,committed;
static emos_file_cancel cancel_fn;
static void *cancel_user;
static void poll_cancel(void){if(cancel_fn && cancel_fn(cancel_user))cancelled=1;}
void service_progress(void){poll_cancel();}
static int path_valid(const char *p,unsigned *length){
    unsigned i,start=1;
    if(!p || p[0]!='/')return 0;
    for(i=1;i<=120;i++){
        unsigned char c=p[i];
        if(c && (c<32 || c>126 || c==':' || c=='\\' || c=='*' || c=='?'))return 0;
        if(c=='/' || !c){
            unsigned n=i-start;
            if(!n || (n==1 && p[start]=='.') || (n==2 && p[start]=='.' && p[start+1]=='.') ||
               p[i-1]=='.' || p[i-1]==' ')return 0;
            start=i+1;
        }
        if(!c){*length=i;return 1;}
    }
    return 0;
}
static int valid_record(const uint8_t *p,unsigned n){
    if(n<20 || n>240 || p[0]!='S'||p[1]!='D'||p[2]!=1 || sd_u16(p+14)!=n-20)return 0;
    uint32_t c=sd_crc_update(UINT32_C(0xffffffff),p,16);
    return (sd_crc_update(c,p+20,n-20)^UINT32_C(0xffffffff))==sd_u32(p+16);
}
static unsigned exchange(unsigned length,unsigned kind,unsigned *received){
    unsigned got,spins=1000000;
    uint32_t start=sdapp_clock();
    if(sdapp_link(2,tx,length,NULL,0,&got))return EMOS_FILE_UNCERTAIN;
    do {
        poll_cancel();
        if(sdapp_link(1,NULL,0,rx,sizeof rx,&got))return EMOS_FILE_UNCERTAIN;
        if(got){
            if(!valid_record(rx,got) || rx[3]!=kind || memcmp(rx+4,tx+4,9))return EMOS_FILE_PROTOCOL;
            *received=got;return EMOS_FILE_OK;
        }
    }while(--spins && (uint32_t)(sdapp_clock()-start)<24);
    return EMOS_FILE_UNCERTAIN;
}
static unsigned control(unsigned op,unsigned result,const uint8_t *extra,unsigned n){
    unsigned got,r;
    if(n>192 || sequence==UINT32_MAX)return EMOS_FILE_PROTOCOL;
    sd_header(tx,4,token,++sequence,op,result,28+n);memcpy(tx+20,prefix,28);
    if(op==1){memset(tx+20,0,28);tx[46]=1;}
    if(n)memcpy(tx+48,extra,n);
    sd_seal(tx);r=exchange(48+n,5,&got);if(r)return r;
    if(got!=48)return EMOS_FILE_PROTOCOL;
    if(op==1){
        static const uint8_t zero[18]={0};
        if(rx[13])return EMOS_FILE_UNAVAILABLE;
        if(!memcmp(rx+20,zero,8) || memcmp(rx+28,zero,18) || rx[47] || (rx[46]&0xf0))return EMOS_FILE_PROTOCOL;
        if((rx[46]&9)!=9)return EMOS_FILE_UNAVAILABLE;
        memcpy(prefix,rx+20,8);return 0;
    }
    if(memcmp(rx+20,prefix,12)||memcmp(rx+36,prefix+16,12))return EMOS_FILE_PROTOCOL;
    if(op==5 && !sd_u32(prefix+12)){
        if(rx[13]==2)return EMOS_FILE_BUSY;
        if(rx[13]==5 || rx[13]==3)return EMOS_FILE_UNAVAILABLE;
        if(!sd_u32(rx+32))return EMOS_FILE_PROTOCOL;
        memcpy(prefix+12,rx+32,4);claimed=1;
    }else if(memcmp(rx+32,prefix+12,4))return EMOS_FILE_PROTOCOL;
    if(rx[13])return rx[13]==2?EMOS_FILE_BUSY:rx[13]==9?EMOS_FILE_CANCELLED:EMOS_FILE_IO;
    return 0;
}
static unsigned remote_rpc(unsigned op,const uint8_t *p,unsigned n,unsigned *out_n){
    unsigned got,r;
    if(file_sequence==UINT32_MAX || n>220)return EMOS_FILE_PROTOCOL;
    sd_header(tx,SD_REQUEST,file_session,++file_sequence,op,0,n);
    if(n)memcpy(tx+20,p,n);sd_seal(tx);r=exchange(n+20,SD_RESPONSE,&got);if(r)return r;
    *out_n=got-20;
    return rx[13]?EMOS_FILE_IO:0;
}
static unsigned local_rpc(unsigned op,const uint8_t *p,unsigned n,unsigned *out_n){
    sd_header(tx,SD_REQUEST,token,++local_sequence,op,0,n);
    if(n)memcpy(tx+20,p,n);sd_seal(tx);
    unsigned got=service_request(tx,n+20,local_rx);if(!got)return EMOS_FILE_PROTOCOL;
    *out_n=got-20;return local_rx[13]?EMOS_FILE_IO:0;
}
static unsigned rpc(int remote,unsigned op,const uint8_t *p,unsigned n,unsigned *got){
    return remote?remote_rpc(op,p,n,got):local_rpc(op,p,n,got);
}
static uint8_t *payload(int remote){return (remote?rx:local_rx)+20;}
static unsigned file_read(int remote,const char *path,uint32_t at,uint32_t *size,unsigned *count){
    uint8_t p[127];unsigned n=strlen(path),got,r;
    sd_put32(p,at);sd_put16(p+4,sizeof chunk);p[6]=n;memcpy(p+7,path,n);
    r=rpc(remote,SD_READ,p,n+7,&got);if(r)return r;
    if(got<4 || got>216)return EMOS_FILE_PROTOCOL;
    *size=sd_u32(payload(remote));*count=got-4;
    if(at>*size || *count!=(*size-at<sizeof chunk?*size-at:sizeof chunk))return EMOS_FILE_PROTOCOL;
    memcpy(chunk,payload(remote)+4,*count);return 0;
}
unsigned emos_file_transfer(unsigned direction,const char *source,const char *destination,
                            unsigned overwrite,emos_file_cancel cancel,void *user){
    unsigned sl,dl,r=EMOS_FILE_INVALID,got=0,n,at,remote_source=direction==EMOS_FILE_RECEIVE;
    uint8_t data[220];uint32_t size=0,total=0,offset=0,crc=UINT32_C(0xffffffff),tid;
    if(busy)return EMOS_FILE_BUSY;
    if((direction!=3 && direction!=4)||overwrite>1||!path_valid(source,&sl)||
       !path_valid(destination,&dl)||dl>112)return EMOS_FILE_INVALID;
    busy=1;cancel_fn=cancel;cancel_user=user;
    cancelled=opened=initialized=claimed=local_stage=remote_stage=committed=0;
    sequence=file_sequence=local_sequence=0;memset(prefix,0,sizeof prefix);
    descriptor[0]=direction;descriptor[1]=overwrite;sd_put16(descriptor+2,sl);sd_put16(descriptor+4,dl);
    descriptor[6]=descriptor[7]=0;memcpy(descriptor+8,source,sl);memcpy(descriptor+8+sl,destination,dl);
    /* Snapshot caller paths before transport. No caller pointer is retained. */
    char src[121],dst[121];memcpy(src,descriptor+8,sl);src[sl]=0;
    memcpy(dst,descriptor+8+sl,dl);dst[dl]=0;
    poll_cancel();if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
    r=sdapp_link(4,NULL,0,rx,4,&got);
    if(r){r=r==31?EMOS_FILE_BUSY:EMOS_FILE_UNAVAILABLE;goto done;}
    opened=1;if(got!=4 || !(token=sd_u32(rx))){r=EMOS_FILE_PROTOCOL;goto done;}
    sd_put32(prefix+8,token);sd_put32(prefix+16,token);sd_put32(prefix+20,token);
    prefix[24]=2;prefix[25]=direction;
    if((r=control(1,0,NULL,0))){if(r==EMOS_FILE_UNCERTAIN)r=EMOS_FILE_UNAVAILABLE;goto done;}
    n=sl+dl+8;uint32_t desc_crc=sd_crc(descriptor,n);
    for(at=0;at<n;){unsigned take=n-at;if(take>184)take=184;
        sd_put16(data,n);sd_put16(data+2,at);sd_put32(data+4,desc_crc);memcpy(data+8,descriptor+at,take);
        if((r=control(5,0,data,take+8)))goto done;at+=take;
    }
    if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
    if((r=control(4,0,NULL,0)))goto done;
    file_session=sd_crc(prefix,sizeof prefix);if(!file_session)file_session=1;
    if(!service_init("/",token)){r=EMOS_FILE_INVALID;goto done;}
    initialized=1;
    if((r=local_rpc(SD_HELLO,NULL,0,&got)))goto done;
    if((r=remote_rpc(SD_HELLO,NULL,0,&got)))goto done;
    if(got!=8 || sd_u16(rx+24)!=212 || (sd_u16(rx+26)&15)!=15 || (sd_u16(rx+26)&16)){r=EMOS_FILE_PROTOCOL;goto done;}
    /* First checked pass computes source length/CRC without retaining whole file. */
    do {
        if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
        if((r=file_read(remote_source,src,offset,&size,&got)))goto done;
        if(offset && size!=total){r=EMOS_FILE_PROTOCOL;goto done;}total=size;
        crc=sd_crc_update(crc,chunk,got);offset+=got;
    }while(offset<total);
    crc^=UINT32_C(0xffffffff);
    if(!overwrite){
        data[0]=dl;memcpy(data+1,dst,dl);
        r=rpc(!remote_source,SD_STAT,data,dl+1,&got);
        if(!r){r=EMOS_FILE_IO;goto done;}
        uint8_t *reply=remote_source?local_rx:rx;
        if(r!=EMOS_FILE_IO || reply[13]!=SD_FILE_ERROR || got!=1 || (reply[20]!=4 && reply[20]!=5))goto done;
    }
    if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
    tid=token;sd_put32(data,tid);sd_put32(data+4,total);sd_put32(data+8,crc);
    data[12]=dl;memcpy(data+13,dst,dl);
    /* BEGIN may mutate before an acknowledgement is lost: retain recovery. */
    if(remote_source)local_stage=1;else remote_stage=1;
    if((r=rpc(!remote_source,SD_BEGIN,data,dl+13,&got)))goto done;
    if(got!=8 || sd_u32(payload(!remote_source))!=tid || sd_u32(payload(!remote_source)+4)){r=EMOS_FILE_PROTOCOL;goto done;}
    for(offset=0;offset<total;offset+=got){
        if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
        unsigned count;
        if((r=file_read(remote_source,src,offset,&size,&count)))goto done;
        if(size!=total){r=EMOS_FILE_PROTOCOL;goto done;}
        sd_put32(data,tid);sd_put32(data+4,offset);memcpy(data+8,chunk,count);
        if((r=rpc(!remote_source,SD_WRITE,data,count+8,&got)))goto done;
        if(got!=8 || sd_u32(payload(!remote_source))!=tid || sd_u32(payload(!remote_source)+4)!=offset+count){r=EMOS_FILE_PROTOCOL;goto done;}
        got=count;
    }
    if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
    sd_put32(data,tid);
    if((r=rpc(!remote_source,SD_FINISH,data,4,&got)))goto done;
    if(got!=8 || sd_u32(payload(!remote_source))!=total || sd_u32(payload(!remote_source)+4)!=crc){r=EMOS_FILE_PROTOCOL;goto done;}
    if(cancelled){r=EMOS_FILE_CANCELLED;goto done;}
    if((r=rpc(!remote_source,SD_ACTIVATE,data,4,&got)))goto done;
    if(got){r=EMOS_FILE_PROTOCOL;goto done;}committed=1;
    data[0]=1;r=control(9,0,data,1);
done:
    if(initialized)service_stop();
    if(claimed){
        if(r){data[0]=committed?1:(local_stage||remote_stage)?3:0;
            /* No unsafe cleanup; the peer and operator retain recovery state. */
            (void)control(8,0,NULL,0);(void)control(9,r==EMOS_FILE_CANCELLED?9:7,data,1);
        }
        unsigned close_result=control(10,0,NULL,0);if(!r && close_result)r=EMOS_FILE_UNCERTAIN;
    }
    if(opened){unsigned ignored;if(sdapp_link(3,NULL,0,NULL,0,&ignored) && !r)r=EMOS_FILE_UNCERTAIN;}
    if(r && (local_stage||remote_stage) && !committed)r=EMOS_FILE_RECOVERY;
    cancel_fn=NULL;cancel_user=NULL;busy=0;return r;
}
