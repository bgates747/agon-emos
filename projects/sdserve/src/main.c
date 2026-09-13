/* Ordinary foreground MOS application. No mode switch, UART takeover, ISR
 * filesystem work, automatic firmware flashing, or game execution. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "service.h"
#ifndef SDSERVE_BUILD_ID
#define SDSERVE_BUILD_ID "UNVERSIONED-sdserve-development"
#define SDSERVE_STATUS "experimental"
#endif
extern uint24_t emos_gateway_call(uint8_t *request);
static uint8_t gateway[66],input[SD_MAX_RECORD+1],incoming[SD_MAX_RECORD];
static uint8_t reply[SD_MAX_RECORD],presence[SD_HEADER];
static uint32_t boot,last_presence;
static uint8_t transport_failed;
static void put24(uint8_t *p,uint24_t value) { p[0]=value;p[1]=value>>8;p[2]=value>>16; }
static unsigned link_call(uint8_t operation,const uint8_t *record,unsigned length) {
    input[0]=operation;if(length) memcpy(input+1,record,length);
    memset(gateway,0,sizeof(gateway));gateway[0]=66;gateway[2]=1;
    gateway[4]=3;gateway[5]=6;gateway[6]=2;
    memcpy(gateway+25,"ext",3);memcpy(gateway+41,"sdlink",6);
    put24(gateway+10,(uint24_t)input);put24(gateway+13,length+1);
    if(operation==1) { put24(gateway+16,(uint24_t)incoming);put24(gateway+19,sizeof(incoming)); }
    return emos_gateway_call(gateway);
}
static void announce(uint8_t closing) {
    sd_header(presence,SD_PRESENCE,boot,0,closing,0,0);sd_seal(presence);
    if(link_call(2,presence,sizeof(presence))) transport_failed=1;
}
void service_progress(void) {
    uint32_t now=getsysvar_time();
    // MOS time advances by two units per VBlank (120 units/second here).
    if(!transport_failed && (uint32_t)(now-last_presence)>=120) {
        last_presence=now;announce(0);
    }
}
int main(int argc,char **argv) {
    const char *scope=argc>1?argv[1]:"/extender/sdtest";
    const volatile uint8_t *sv=mos_sysvars();
    if(argc>2) { puts("Usage: sdserve.bin [absolute-root]");return 1; }
    boot=getsysvar_time()^UINT32_C(0x173d5a91);if(!boot) boot=1;
    if(!service_init(scope,boot)) { puts("Invalid SD service root");return 1; }
    unsigned error=link_call(0,NULL,0);
    if(error) {
        printf("SD service unavailable (EMOS %u). Requires ext.sdlink and Extender keyboard.\n",error);
        return 1;
    }
    printf("%s (%s), root %s\nEscape returns to MOS.\n",SDSERVE_BUILD_ID,SDSERVE_STATUS,scope);
    last_presence=getsysvar_time();announce(0);
    while(!transport_failed && !service_exiting()) {
        if(sv[sysvar_vkeydown] && sv[sysvar_keyascii]==27) break;
        if(link_call(1,NULL,0)) { transport_failed=1;break; }
        unsigned n=gateway[22];
        if(n) {
            n=service_request(incoming,n,reply);
            if(n && link_call(2,reply,n)) transport_failed=1;
        }
        service_progress();
    }
    service_stop();
    if(!transport_failed) announce(1);
    (void)link_call(3,NULL,0);
    puts(transport_failed?"SD transport failed; stage preserved for recovery.":"SD service stopped.");
    return transport_failed?1:0;
}
