/* R05-A11 actual card-transfer fixture. Mode selection belongs to the CLI.
 * Ten-second ordinary-app windows before/after transfer test external refusal.
 * Only fixture-owned paths are written; caller state is checked afterward. */
#include <agon/mos.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sdapp.h"
static uint8_t sentinel[4096];
static void wait_ticks(uint32_t ticks){uint32_t at=getsysvar_time();while((uint32_t)(getsysvar_time()-at)<ticks){}}
int main(int argc,char **argv){
    char local[100],remote[100],result_path[100],result[180];
    const char *label=argc==2?argv[1]:"";
    if(strcmp(label,"legacy")&&strcmp(label,"excom")){puts("Use RUN legacy or RUN excom");return 1;}
    snprintf(local,sizeof local,"/agents/extender/results/app-%s-in.bin",label);
    snprintf(remote,sizeof remote,"/agents/extender/app-%s-out.bin",label);
    snprintf(result_path,sizeof result_path,"/agents/extender/results/app-%s.txt",label);
    for(unsigned i=0;i<sizeof sentinel;++i)sentinel[i]=(uint8_t)(i*17+3);
    puts("app-transfer-probe-r02: application active; ten-second exclusion window");
    uint32_t start=getsysvar_time();wait_ticks(1200);
    unsigned receive=emos_file_transfer(3,"/agents/extender/app-source.bin",local,0,NULL,NULL);
    unsigned send=receive?99:emos_file_transfer(4,local,remote,0,NULL,NULL);
    unsigned memory=1;
    for(unsigned i=0;i<sizeof sentinel;++i)if(sentinel[i]!=(uint8_t)(i*17+3))memory=0;
    puts("Application transfers ended; ten-second exclusion window");wait_ticks(1200);
    snprintf(result,sizeof result,"app-transfer-probe-r02 %s receive=%u send=%u sentinel=%u ticks=%lu\n",label,receive,send,memory,(unsigned long)(getsysvar_time()-start));
    if(mos_save(result_path,result,strlen(result)))return 1;
    printf("%s",result);return receive||send||!memory;
}
