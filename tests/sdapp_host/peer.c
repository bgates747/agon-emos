#include <assert.h>
#include <string.h>
#include "sdapp.h"
#include "sd_wire.h"
extern int peer_init(const char *,uint32_t);
extern unsigned peer_request(const uint8_t *,unsigned,uint8_t *);
extern void peer_stop(void);
static uint8_t reply[240],prefix[28],desc[248];
static unsigned pending,owned,mode,ready,offset,total,opens,closes,writes,controls;
static uint32_t tick,desc_crc;
void peer_progress(void){}
void mock_reset(unsigned m){peer_stop();mode=m;pending=owned=ready=offset=total=opens=closes=writes=controls=0;tick=0;memset(prefix,0,28);}
unsigned mock_count(unsigned n){return n==0?owned:n==1?opens:n==2?closes:n==3?writes:controls;}
uint32_t sdapp_clock(void){return mode==7?0:tick++;}
unsigned sdapp_link(unsigned op,const uint8_t *p,unsigned n,uint8_t *out,unsigned cap,unsigned *got){
    *got=0;
    if(op==4){assert(cap==4 && !owned);owned=1;++opens;sd_put32(out,19);*got=4;return 0;}
    if(op==3){assert(owned);owned=0;++closes;pending=0;peer_stop();return 0;}
    assert(owned);
    if(op==1){assert(cap==240);if(pending){*got=pending;memcpy(out,reply,pending);pending=0;}return 0;}
    assert(op==2 && n>=20 && n<=240 && !pending);
    assert(sd_u16(p+14)==n-20);
    if(p[3]==4){
        ++controls;assert(n>=48);memcpy(reply,p,48);reply[3]=5;sd_put16(reply+14,28);
        if(p[12]==1){memset(reply+20,0,28);reply[20]=71;reply[46]=mode==5?5:13;offset=total=ready=0;}
        else {
            assert(p[44]==2 && p[45]>=3 && p[45]<=4 && sd_u32(p+28)==19);
            if(p[12]==5){
                if(mode==4){reply[13]=2;}else{
                    assert(n>=56 && sd_u16(p+50)==offset);
                    if(!offset){total=sd_u16(p+48);desc_crc=sd_u32(p+52);assert(total<=248);}
                    assert(sd_u16(p+48)==total && sd_u32(p+52)==desc_crc && offset+n-56<=total);
                    memcpy(desc+offset,p+56,n-56);offset+=n-56;sd_put32(reply+32,73);
                    if(offset==total)assert(sd_crc(desc,total)==desc_crc);
                }
            }else if(p[12]==4){assert(offset==total && total>8);ready=1;memcpy(prefix,p+20,28);assert(peer_init("/p4",91));}
            else if(p[12]==8)reply[13]=9;
            else assert(p[12]==9 || p[12]==10);
        }
        if(mode==3 && p[12]==4)reply[36]^=1;
        pending=48;sd_seal(reply);
    }else {
        assert(p[3]==1 && ready && sd_valid(p,n));
        uint32_t sid=sd_crc(prefix,28);if(!sid)sid=1;assert(sd_u32(p+4)==sid);
        pending=peer_request(p,n,reply);assert(pending);
        if(p[12]==6)++writes;
        if(mode==6 && p[12]==8){pending=0;return 0;}
    }
    if(mode==7){pending=0;return 0;}
    if(mode==8){reply[3]=1;sd_seal(reply);}
    if(mode==9 && p[3]==4 && p[12]==5){reply[44]=1;sd_seal(reply);}
    if(mode==1)reply[16]^=1;
    if(mode==2){reply[8]^=1;sd_seal(reply);}
    return 0;
}
