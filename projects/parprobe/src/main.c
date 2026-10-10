/* PORT-008 LC03-A private qualification MOSlet. EMOS owns every transport
 * operation through API51h; no GPIO, register, vector or resident-address calls.
 * Needs paired private probe builds, admitted extender input and prepared
 * /agents/extender/results directory. Never invoked automatically by a build.
 */
#include <agon/mos.h>
#include <stdio.h>
#include <string.h>
#include "pattern.h"
extern uint24_t emos_gateway_call(uint8_t *request);
static uint8_t request[66],input[2],storage[4114];
static void put24(uint8_t *p,uint24_t n){p[0]=n;p[1]=n>>8;p[2]=n>>16;}
static unsigned call(unsigned n,unsigned reverse){
    memset(request,0,sizeof request);request[0]=66;request[2]=1;
    request[4]=3;request[5]=8;request[6]=2;
    memcpy(request+25,"ext",3);memcpy(request+41,"uartdiag",8);
    input[0]=6;input[1]=reverse;
    put24(request+10,(uint24_t)input);request[13]=2;
    put24(request+16,(uint24_t)(storage+1));put24(request+19,n+16);
    return emos_gateway_call(request);
}
int main(int argc,char **argv){
    static const unsigned sizes[]={1,2,255,256,4096};
    uint32_t started=getsysvar_time();unsigned row=0,passed=0;
    FILE *f;
    if(argc==2 && !strcmp(argv[1],"--version")) {
        puts("parprobe r01: private native two-way EMOSlet; not ordinary uartdiag");return 0;
    }
    if(argc!=2 || strncmp(argv[1],"/agents/extender/results/",25))return 19;
    f=fopen(argv[1],"r");if(f){fclose(f);puts("Result already exists");return 19;}
    f=fopen(argv[1],"w");if(!f){puts("Cannot create result");return 15;}
    fprintf(f,"# start_mos_ticks=%lu; nominal120_Hz_not_calibrated\n",(unsigned long)started);
    fputs("row,direction,length,descriptor,crc,first,last,guards,pattern,service_status,elapsed_mos_ticks\n",f);
    if(ferror(f) || fflush(f)){fclose(f);return 15;}
    for(unsigned round=0;round<2;++round)for(unsigned reverse=0;reverse<2;++reverse)
    for(unsigned j=0;j<sizeof sizes/sizeof sizes[0];++j){
        unsigned n=sizes[j],guards,pattern,status;uint32_t before,elapsed,crc=0;
        uint8_t *out=storage+1,*data=out+16,seed=(uint8_t)(17+round*109+j*11);
        memset(storage,0xCC,sizeof storage);storage[0]=0xA5;storage[n+17]=0x5A;
        if(!reverse)for(unsigned i=0;i<n;++i)data[i]=parallel_probe_pattern(i,seed);
        before=getsysvar_time();status=call(n,reverse);elapsed=getsysvar_time()-before;
        guards=storage[0]==0xA5 && storage[n+17]==0x5A;
        pattern=guards && !status && request[22]==(uint8_t)(n+16) &&
            request[23]==(uint8_t)((n+16)>>8) && !request[24] &&
            !memcmp(out,"EX\x02\x02",4) && out[8]==1 && !out[9] &&
            out[10]==(uint8_t)n && out[11]==(uint8_t)(n>>8) && out[12]==reverse && !out[13];
        if(pattern){
            if(reverse)seed=parallel_probe_seed(out);
            for(unsigned i=0;i<n;++i)if(data[i]!=parallel_probe_pattern(i,seed))pattern=0;
            crc=parallel_probe_crc(data,n);
        }
        fprintf(f,"%u,%u,%u,",++row,reverse,n);
        for(unsigned i=0;i<16;++i)fprintf(f,"%02x",out[i]);
        fprintf(f,",%08lx,%02x,%02x,%u,%u,%u,%lu\n",(unsigned long)crc,
            data[0],data[n-1],guards,pattern,status,(unsigned long)elapsed);
        if(ferror(f) || fflush(f) || !pattern)goto done;
        ++passed;printf("Parallel probe %u/20 passed: %s %u bytes\r\n",row,reverse?"P4 to Agon":"Agon to P4",n);
    }
 done:
    fprintf(f,"# end_mos_ticks=%lu; total_mos_ticks=%lu; passed=%u/20\n",
        (unsigned long)getsysvar_time(),(unsigned long)(getsysvar_time()-started),passed);
    { unsigned error=ferror(f);if(fclose(f) || error)return 15; }
    printf("Parallel probe %s: %u/20; return to MOS\r\n",passed==20?"passed":"failed",passed);
    return passed==20?0:15;
}
