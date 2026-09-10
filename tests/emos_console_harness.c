/* Real resident console adapter, scripted UART/IRQ boundary (not VDP rendering). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emos_console.h"
#include "emos_console_wire.h"
#include "emos_keyboard.h"

volatile BYTE emosVduBackend, emos_key_faulted;
BYTE vdp_protocol_data[16];
static BYTE irq=1, tick, suppress, wrong_nonce, wrong_crc, frozen, claimed;
static BYTE tx[128], mode[8], cursor[2];
static unsigned tx_count, effects, release_count;
BYTE emos_keyboard_clock(void) { if (!frozen) tick+=2; return tick; }
BYTE emos_keyboard_lock(void) { BYTE old=irq;irq=0;return old; }
void emos_keyboard_unlock(BYTE enabled) { irq=enabled; }
BYTE emos_keyboard_transport_claim(void) { claimed=1;return 0; }
void emos_keyboard_transport_release(void) { claimed=0;++release_count; }
void emos_mainboard_put(BYTE b) { (void)b; }
void emos_vdp_effect(BYTE command) {
    assert(!irq);++effects;
    if(command==0x86)memcpy(mode,vdp_protocol_data,8);
    if(command==0x82)memcpy(cursor,vdp_protocol_data,2);
}
static void receive(BYTE command,BYTE *p,BYTE n) {
    BYTE previous=emos_keyboard_lock();
    emos_console_packet(command,p,n);
    emos_keyboard_unlock(previous);
}
BYTE emos_keyboard_send(const BYTE *p,UINT16 n) {
    assert(claimed);
    if(n==CONSOLE_SIZE) {
        BYTE reply[CONSOLE_SIZE];
        assert(console_valid(p));memcpy(reply,p,sizeof(reply));
        reply[3]|=0x80;
        if(p[3]==CONSOLE_PREPARE)memcpy(reply+8,(BYTE[]){1,2,3,4},4);
        else if(wrong_nonce)++reply[8];
        console_seal(reply);
        if(wrong_crc)reply[14]^=1;
        if(!suppress)receive(CONSOLE_REPLY,reply,sizeof(reply));
    } else if(n==15 && p[0]==26) {
        BYTE m[]={0x80,2,0xe0,1,80,60,16,0}, c[]={2,3};
        receive(0x86,m,8);receive(0x82,c,2);
        BYTE token=p[14];receive(0x80,&token,1);
    } else if(n!=3) {assert(tx_count+n<sizeof(tx));memcpy(tx+tx_count,p,n);tx_count+=n;}
    return EMOS_KEY_OK;
}
int main(void) {
    memset(vdp_protocol_data,0xA5,16);
    assert(emos_console_write_byte('x')!=0);
    suppress=1;
    assert(!emos_console_prepare());assert(!emosVduBackend && !effects);
    assert(emos_console_recover());assert(!claimed && !emos_console_owned);
    frozen=1;assert(!emos_console_prepare());assert(emos_console_recover());frozen=0;
    suppress=0;wrong_crc=1;assert(!emos_console_prepare());assert(emos_console_recover());wrong_crc=0;
    for(unsigned run=0;run<3;++run) {
        assert(emos_console_prepare());assert(emos_console_owned);
        unsigned before=effects;
        assert(emos_console_commit());assert(effects==before); /* staged, not published */
        emosVduBackend=1;emos_console_publish();assert(effects==before+2);
        assert(mode[4]==80 && mode[5]==60 && cursor[0]==2 && cursor[1]==3);
        for(unsigned i=0;i<16;++i)assert(vdp_protocol_data[i]==0xA5);
        assert(!emos_console_write_byte('a'));
        BYTE invalid_mode[]={1,2,3,4,5,6,7};receive(0x86,invalid_mode,7);assert(effects==before+2);
        BYTE key[]={65,0,20,1};receive(0x81,key,4);assert(effects==before+2);
        wrong_nonce=1;assert(!emos_console_recover());assert(emos_console_owned && claimed);
        wrong_nonce=0;assert(emos_console_recover());emosVduBackend=0;
        assert(!emos_console_owned && !claimed);
        BYTE late[]={9,9};receive(0x82,late,2);assert(cursor[0]==2);
    }
    assert(tx_count==3 && !memcmp(tx,"aaa",3));assert(release_count==6);
    puts("Console control, staging, private scratch, stale replies and bounded recovery passed");
}
