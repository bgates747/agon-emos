/* INTEG-014 E06 diagnostic application, NEVER a supported routing API.
 * Fixed-ROM observer; restores both interrupt owners before MOS transitions.
 * No mode switch, SD, display output or formatting inside a measured interval.
 */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "firmware_abi.h"
extern uint24_t bench_clock(const volatile uint8_t *);
extern uint24_t bench_count(const uint8_t *,uint24_t);
extern void graphics_callback(void),diag0_irq(void),diag1_irq(void);
extern uint8_t diag_lock(void),diag_rom_first(void);
extern void diag_unlock(uint8_t),diag_uart0_put(uint8_t);
volatile uint24_t diag0_count,diag1_count,diag0_left;
volatile uint8_t * volatile diag0_cursor;
volatile uint8_t diag0_overflow;
static uint8_t raw0[8192],data[65535],returned[2048];
static volatile uint8_t *sv;
static volatile uint8_t armed,bad,seen[4];
static volatile uint16_t owner,packets;
static volatile uint32_t value[4],count[4];
static volatile uint24_t last_rx;
static unsigned repeat,load,kind,length,saved,failures;
static uint16_t token;
static FIL file;
static char filename[32],line[512],command[64];
static uint32_t rng;
static uint24_t ticks(void){return bench_clock(sv);}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint8_t next_byte(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (uint8_t)rng;}
static uint32_t crc32(const uint8_t *p,unsigned n){uint32_t crc=0xffffffffUL;while(n--){crc^=*p++;for(unsigned j=0;j<8;++j)crc=(crc>>1)^((crc&1)?0xedb88320UL:0);}return ~crc;}
void graphics_receive(const uint8_t *p){
    if(!armed || p[0]!='Q' || p[1]!='T' || p[2]!='G' || p[3]!=0xA1 || p[7]!=1)return;
    unsigned id=(unsigned)p[4]|((unsigned)p[5]<<8),k=p[6];
    if(k==96){if(id!=packets || packets>=256){bad=1;return;}memcpy(returned+packets*8,p+8,8);++packets;last_rx=ticks();return;}
    if(id!=owner || k>3)return;
    if(seen[k]){bad=1;return;}value[k]=u32(p+8);count[k]=u32(p+12);last_rx=ticks();seen[k]=1;
}
static void arm(void){armed=0;owner=++token;bad=0;packets=0;memset((void*)seen,0,sizeof seen);memset((void*)value,0,sizeof value);memset((void*)count,0,sizeof count);armed=1;}
static unsigned send(const uint8_t *p,unsigned n){return bench_count(p,n);}
static unsigned request(unsigned op){uint8_t p[]={23,0,0xEE,op,token,token>>8,length,length>>8,3};return send(p,sizeof p);}
static unsigned wait_for(unsigned k){uint24_t t=ticks();uint32_t budget=16000000UL;while(!seen[k] && !bad && ticks()-t<1800 && --budget){}return (!seen[k]||bad)?FR_TIMEOUT:0;}
static unsigned install(void){
    uint8_t irq=diag_lock();
    if(!irq || *(volatile uint24_t*)ROM_VECTOR0!=ROM_UART0_IRQ || *(volatile uint24_t*)ROM_VECTOR1!=ROM_UART1_IRQ){diag_unlock(irq);return FR_INT_ERR;}
    diag0_count=diag1_count=0;diag0_overflow=0;diag0_cursor=raw0;diag0_left=sizeof raw0;
    mos_setintvector(0x18,diag0_irq);mos_setintvector(0x1A,diag1_irq);
    diag_unlock(irq);return 0;
}
static unsigned restore(void){
    uint8_t irq=diag_lock();
    if(*(volatile uint24_t*)ROM_VECTOR0!=(uint24_t)diag0_irq || *(volatile uint24_t*)ROM_VECTOR1!=(uint24_t)diag1_irq){diag_unlock(irq);return FR_INT_ERR;}
    mos_setintvector(0x18,(void(*)(void))ROM_UART0_IRQ);mos_setintvector(0x1A,(void(*)(void))ROM_UART1_IRQ);
    diag_unlock(irq);return 0;
}
/* Indexed loads avoid the observed AgonDev -Oz postincrement-argument
 * miscompile: the first probe emitted INC IY before loading the byte. */
static void uart0(const uint8_t *p,unsigned n){for(unsigned i=0;i<n;++i)diag_uart0_put(p[i]);}
static void burst(uint16_t id){uint8_t p[]={23,0,0xEE,4,id,id>>8,0,1,3};uart0(p,sizeof p);}
static void query(void){const uint8_t p[]={23,0,0x86};uart0(p,sizeof p);}
static unsigned raw_size(void){return sizeof raw0-diag0_left;}
static unsigned validate0(unsigned queries,uint16_t id){
    unsigned n=raw_size(),errors=0;
    if(diag0_overflow)return 1;
    if(load==0)return n!=0;
    if(load==1){
        const uint8_t expected[]={0x86,8,0,2,128,1,64,48,64,20};
        if(n!=queries*sizeof expected)return 1;
        for(unsigned i=0;i<n;i+=sizeof expected)if(memcmp(raw0+i,expected,sizeof expected))++errors;
        return errors;
    }
    if(n!=4626)return 1;
    rng=0x12345678UL;
    for(unsigned i=0;i<257;++i){
        const uint8_t *p=raw0+i*18;
        if(memcmp(p,"\x8C\x10QTG\xA1",6) || p[9]!=0){++errors;continue;}
        unsigned seq=(unsigned)p[6]|((unsigned)p[7]<<8);
        if(i<256){if(seq!=i || p[8]!=96)++errors;for(unsigned j=0;j<8;++j)if(p[10+j]!=next_byte())++errors;}
        else if(seq!=id || p[8]!=3 || u32(p+14)!=256)++errors;
    }
    return errors;
}
static unsigned write_line(const char *s){unsigned n=strlen(s),r=ffs_fwrite(&file,s,n)==n?0:FR_DISK_ERR;unsigned sync=ffs_fsync(&file),close=ffs_fclose(&file);return r?r:(sync?sync:close);}
static unsigned append(const char *s){unsigned r=ffs_fopen(&file,filename,FA_WRITE|FA_OPEN_APPEND);return r?r:write_line(s);}
static unsigned create(void){for(unsigned n=1;n<1000;++n){snprintf(filename,sizeof filename,"DUT%03u.CSV",n);unsigned s=ffs_fopen(&file,filename,FA_WRITE|FA_CREATE_NEW);if(s==FR_EXIST)continue;if(s)return s;snprintf(line,sizeof line,"# %s; ROM %s\r\nrepeat,load,kind,length,total_ticks,send_ticks,reply_ticks,endpoint_us,uart0_irqs,uart1_irqs,uart0_bytes,queries,uart0_errors,uart1_errors,status,restored\r\n",PROBE_BUILD_ID,ROM_BUILD_ID);return write_line(line);}return FR_EXIST;}
static unsigned row(uint24_t elapsed,uint24_t sent,uint24_t reply,uint32_t endpoint,unsigned queries,unsigned e0,unsigned e1,unsigned status,unsigned restored){
    snprintf(line,sizeof line,"%u,%u,%u,%u,%u,%u,%u,%lu,%u,%u,%u,%u,%u,%u,%u,%u\r\n",repeat,load,kind,length,elapsed,sent,reply,(unsigned long)endpoint,diag0_count,diag1_count,raw_size(),queries,e0,e1,status,restored);
    unsigned result=append(line);++saved;if(status||result)++failures;
    /* Preserve raw UART0 observations outside the timed interval. */
    char rawname[48];snprintf(rawname,sizeof rawname,"%s-%02u.raw",filename,saved);
    unsigned s=ffs_fopen(&file,rawname,FA_WRITE|FA_CREATE_NEW);
    if(!s){unsigned n=raw_size();s=ffs_fwrite(&file,(const char*)raw0,n)==n?0:FR_DISK_ERR;unsigned c=ffs_fclose(&file);if(!s)s=c;}
    return result?result:(s?s:status);
}
static unsigned trial(void){
    unsigned status=0,queries=0,issued=0,e0=0,e1=0;
    uint24_t start,sent=0,reply=0,at=0,whole;
    uint16_t background=(uint16_t)(0xD000+token+1);
    if(!kind){rng=0x12345678UL;for(unsigned i=0;i<length;++i)data[i]=next_byte();const uint8_t clear[]={23,0,160,10,250,2};status=send(clear,sizeof clear);if(status)return status;}
    arm();
    if(!kind){status=request(1);if(!status)status=wait_for(0);if(!status && (value[0]!=length || count[0]!=3))status=FR_INT_ERR;if(status){armed=0;return status;}}
    status=install();if(status){armed=0;return status;}
    start=ticks();at=start-2;
    if(kind){
        status=request(4);sent=ticks()-start;
        uint32_t budget=16000000UL;
        while(!status && !seen[3] && !bad && ticks()-start<1800 && --budget){
            if(packets && load==2 && !issued){burst(background);issued=1;}
            if(packets && load==1 && ticks()-at>=2){query();++queries;at=ticks();}
        }
        if(!seen[3] || bad)status=FR_TIMEOUT;
        reply=last_rx-start;
    }else{
        if(load==2){burst(background);issued=1;}
        const uint8_t header[]={23,0,160,10,250,0,length,length>>8};status=send(header,sizeof header);
        for(unsigned offset=0;offset<length && !status;){
            unsigned n=length-offset;if(n>256)n=256;
            status=send(data+offset,n);offset+=n;
            if(load==1 && ticks()-at>=2){query();++queries;at=ticks();}
        }
        sent=ticks()-start;
        if(!status)status=request(2);if(!status)status=wait_for(1);
        if(!status)status=request(3);if(!status)status=wait_for(2);
        reply=last_rx-start;
    }
    unsigned expected0=load==2?4626:queries*10;
    uint32_t budget=16000000UL;
    while(raw_size()<expected0 && ticks()-start<1800 && --budget){}
    whole=ticks()-start;
    unsigned restored=restore();armed=0;
    /* Never touch a foreign vector on failure or invoke an EMOS transition
     * while our own UART1 wrapper remains installed. */
    if(restored){for(;;){} /* Foreign owner: no transition, overwrite or batch resume. */}
    e0=validate0(queries,background);
    if(kind){rng=0x12345678UL;for(unsigned i=0;i<packets*8;++i)if(returned[i]!=next_byte())++e1;if(packets!=256 || count[3]!=256)++e1;}
    else {e1=(unsigned)value[2];if(count[1]!=length)++e1;}
    if(bad)++e1;
    if(!status && (e0||e1))status=FR_INT_ERR;
    return row(whole,sent,reply,kind?value[3]:value[1],queries,e0,e1,status,restored);
}
int main(int argc,char **argv){
    sv=(volatile uint8_t*)mos_sysvars();
    /* Read address zero in assembly: a C null dereference is not a ROM API. */
    if(diag_rom_first()!=ROM_FIRST || crc32((const uint8_t*)1,ROM_BYTES-1)!=ROM_CRC32_FROM1){printf("Wrong ROM for UART observer. No vectors changed.\r\n");return 0;}
    unsigned smoke=argc>1 && !strcmp(argv[1],"smoke"),status=create();if(status)return status;
    strcpy(command,"emos excom --keep-display");status=mos_oscli(command,NULL,0);
    mos_setkbvector(graphics_callback,0);
    for(repeat=0;repeat<(smoke?1:3) && !status;++repeat){
        for(load=0;load<(smoke?1:3) && !status;++load){
            for(unsigned k=0;k<(smoke?2:3) && !status;++k){
                kind=k==0;length=kind?256:k==1?4096:65535;
                status=trial();
            }
        }
    }
    armed=0;mos_setkbvector(NULL,0);
    if(*(volatile uint24_t*)ROM_VECTOR0!=ROM_UART0_IRQ || *(volatile uint24_t*)ROM_VECTOR1!=ROM_UART1_IRQ){for(;;){}}
    strcpy(command,"emos legacy --keep-display");unsigned recovery=mos_oscli(command,NULL,0);
    snprintf(line,sizeof line,"# terminal,status=%u,saved=%u,failures=%u,recovery=%u\r\n",status,saved,failures,recovery);unsigned persisted=append(line);
    printf("UART receive tests %s. %u passed, %u failed.\r\n",status||persisted||recovery?"incomplete":"complete",saved-failures,failures);
    return 0;
}
