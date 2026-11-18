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
#if defined(SDSERVE_PARALLEL_UPLOAD)
static uint8_t parallel_slice[512];
#endif
static uint32_t boot,last_presence;
static uint8_t transport_failed,fast_mode;
static void put24(uint8_t *p,uint24_t value) { p[0]=value;p[1]=value>>8;p[2]=value>>16; }
static unsigned link_call(uint8_t operation,const uint8_t *record,unsigned length) {
    input[0]=operation;if(length) memcpy(input+1,record,length);
    memset(gateway,0,sizeof(gateway));gateway[0]=66;gateway[2]=1;
    gateway[4]=3;gateway[5]=6;gateway[6]=2;
    memcpy(gateway+25,"ext",3);memcpy(gateway+41,"sdlink",6);
    put24(gateway+10,(uint24_t)input);put24(gateway+13,length+1);
    if(operation==1) { put24(gateway+16,(uint24_t)incoming);put24(gateway+19,sizeof(incoming)); }
#if defined(SDSERVE_PARALLEL_UPLOAD)
    if(operation==8) {
        put24(gateway+16,(uint24_t)parallel_slice);
        put24(gateway+19,(uint24_t)record[0]|((uint24_t)record[1]<<8)|((uint24_t)record[2]<<16));
    }
#endif
    return emos_gateway_call(gateway);
}
#if defined(SDSERVE_PARALLEL_UPLOAD)
int service_parallel_receive(FIL *file,uint32_t total,uint32_t *written,uint8_t *error) {
    uint8_t args[4],abort=0;
    unsigned status;
    sd_put32(args,total);*written=0;*error=0;
    if(link_call(7,args,4)) { transport_failed=1;return SD_RECOVERY_REQUIRED; }
    while(*written<total) {
        uint24_t got=0,n=total-*written>sizeof(parallel_slice)?sizeof(parallel_slice):(uint24_t)(total-*written);
        put24(args,n);
        status=link_call(8,args,3);
        if(status || ((uint24_t)gateway[22]|((uint24_t)gateway[23]<<8)|((uint24_t)gateway[24]<<16))!=n) { abort=1;break; }
        got=ffs_fwrite(file,(char *)parallel_slice,n);
        *error=ffs_ferror(file);
        *written+=got;
        if(*error || got!=n) { if(!*error)*error=FR_DISK_ERR;abort=1;break; }
        /* CLOCK stays high and VALID low for this FatFS write. No UART
         * presence/ACK, CRC, per-slice negotiation or filesystem ISR work. */
    }
    status=link_call(9,&abort,1);
    if(status) { transport_failed=1;return SD_RECOVERY_REQUIRED; }
    return *error?SD_FILE_ERROR:abort?SD_RECOVERY_REQUIRED:SD_OK;
}
#endif
static void announce(uint8_t closing) {
    sd_header_mode(presence,SD_PRESENCE,boot,0,closing,0,0,fast_mode);sd_seal(presence);
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
    const char *scope="/extender/sdtest";
    int i,fast=0,have_scope=0;
#if defined(SDSERVE_PARALLEL_UPLOAD)
    int parallel=0;
#endif
    const volatile uint8_t *sv=mos_sysvars();
    for(i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--fast") && !fast) fast=1;
#if defined(SDSERVE_PARALLEL_UPLOAD)
        else if(!strcmp(argv[i],"--parallel") && !parallel) parallel=1;
#endif
        else if(argv[i][0]=='/' && !have_scope) { scope=argv[i];have_scope=1; }
        else {
#if defined(SDSERVE_PARALLEL_UPLOAD)
            puts("Usage: sdserve.bin [--fast [--parallel]] [absolute-root]");
#else
            puts("Usage: sdserve.bin [--fast] [absolute-root]");
#endif
            return 1;
        }
    }
    boot=getsysvar_time()^UINT32_C(0x173d5a91);if(!boot) boot=1;
    if(!service_init_mode(scope,boot,fast)) { puts("Invalid SD service root");return 1; }
#if defined(SDSERVE_PARALLEL_UPLOAD)
    if(parallel && !fast) { puts("--parallel requires --fast");return 1; }
    if(parallel)service_enable_parallel();
#endif
    fast_mode=fast;
    if(fast) puts("Fast transfer: no CRCs or whole-file verification.");
    unsigned error=link_call(0,NULL,0);
    if(error) {
        printf("SD service unavailable (EMOS %u). Requires ext.sdlink and Extender keyboard.\n",error);
        return 1;
    }
#if defined(SDSERVE_PARALLEL_UPLOAD)
    if(parallel) {
        uint8_t query[4]={0};
        if(link_call(7,query,4)) {
            (void)link_call(3,NULL,0);
            puts("Parallel upload requires matched EMOS; listener not started.");return 1;
        }
        puts("Parallel uploads selected; one payload admission per file.");
    }
#endif
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
