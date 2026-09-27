/* A05 caller fixture: foreground ADL application; no mode change or loading.
 * Fixed result paths only; controlled peer supplies and verifies 1027 bytes.
 */
#include <agon/mos.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sdapp.h"
static uint8_t sentinel[4096];
static int cancel_now(void *unused){(void)unused;return 1;}
int main(int argc,char **argv){
    unsigned i,r;uint32_t start=getsysvar_time();
    puts("app-transfer-probe-r01 (experimental)");
    int send_only=argc==2 && !strcmp(argv[1],"send");
    for(i=0;i<sizeof sentinel;i++)sentinel[i]=(uint8_t)(i*17+3);
    r=emos_file_transfer(3,"/p4/source.bin","/agents/extender/results/a05-in.bin",0,cancel_now,NULL);
    if(r!=EMOS_FILE_CANCELLED){printf("Early cancellation failed: %u\n",r);return 1;}
    if(!send_only){
        r=emos_file_transfer(3,"/p4/source.bin","/agents/extender/results/a05-in.bin",0,NULL,NULL);
        if(r){printf("Receive failed: %u\n",r);return 1;}
        puts("Application receive PASS");
    }
    r=emos_file_transfer(4,"/agents/extender/results/a05-in.bin","/p4/output.bin",0,NULL,NULL);
    if(r){printf("Send failed: %u\n",r);return 1;}
    for(i=0;i<sizeof sentinel;i++)if(sentinel[i]!=(uint8_t)(i*17+3)){puts("Caller memory changed");return 1;}
    char result[]="A05 application send and caller memory PASS\n";
    if(mos_save("/agents/extender/results/a05-pass.bin",result,sizeof result-1))return 1;
    printf("Application send and caller memory PASS; %lu MOS clock ticks\n",(unsigned long)(getsysvar_time()-start));
    return 0;
}
