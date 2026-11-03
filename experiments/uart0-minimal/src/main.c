/* Finite caller-contract fixture, not a physical performance claim. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packets.h"
#include "identity.h"
extern uint8_t tm_write(void),tm_finish(void),tm_masked_probe(void);
const uint8_t *tm_source;
uint24_t tm_length;
void (*tm_callback)(void);
static volatile uint8_t done;
static volatile unsigned callbacks,keys;
static uint8_t packet[8452],zeros[1024],checks[3840];
static volatile uint8_t *sv;
static uint8_t file;
static void completion(void){callbacks++;done=1;}
static void key_hook(void){keys++;} /* no UART output or further MOS calls */
static void send(const uint8_t*p,unsigned n){mos_puts((const char*)p,n,0);}
static int record(const char*s){puts(s);unsigned n=strlen(s);return mos_fwrite(file,(char*)s,n)==n&&mos_fwrite(file,"\r\n",2)==2;}
#include "run_counter.inc"
static int report_open(void){
 unsigned n=first_report();if(!n)return 0;
 char name[16];
 for(;n<10000;n++){
  snprintf(name,sizeof name,"R%04u.log",n);uint8_t f=mos_fopen(name,FA_READ);
  if(f){mos_fclose(f);continue;}
  file=mos_fopen(name,FA_WRITE|FA_CREATE_NEW);if(!file)return 0;
  return save_next_report(n+1);
 }return 0;
}
/* A new session may legitimately report BUSY until prior printf bytes drain. */
static int start_write(void){
 uint32_t start=getsysvar_time();uint8_t rc;
 do{rc=tm_write();if(rc>1||(uint32_t)(getsysvar_time()-start)>1000)return 0;}while(rc);
 return 1;
}
static int wait_done(void){
 uint32_t start=getsysvar_time();
 while(!done)if((uint32_t)(getsysvar_time()-start)>1000)return 0;
 return 1;
}
static int finish(void){
 uint32_t start=getsysvar_time();uint8_t rc;
 do{rc=tm_finish();if(rc>1||(uint32_t)(getsysvar_time()-start)>1000)return 0;}while(rc);
 return 1;
}
static int gp(const uint8_t*p,unsigned n,uint8_t value){
 sv[sysvar_gp]=0xfe;send(p,n);uint32_t start=getsysvar_time();
 while(sv[sysvar_gp]!=value)if((uint32_t)(getsysvar_time()-start)>1000)return 0;
 return 1;
}
static int verify(void){
 send(v_reset_flag,sizeof v_reset_flag);
 uint8_t f=mos_fopen("checks.bin",FA_READ);if(!f)return 0;
 for(unsigned b=0;b<32;b++){
  if(mos_fread(f,(char*)checks,sizeof checks)!=sizeof checks){mos_fclose(f);return 0;}
  for(unsigned j=0;j<sizeof checks;j+=60){send(checks+j,60);if(!gp(v_reply,sizeof v_reply,0)){mos_fclose(f);return 0;}}
  if((b&7)==7)record("Receiver verification quarter complete");
 }
 mos_fclose(f);return 1;
}
static void fill(void){
 const uint8_t head[]={23,0,128,0x51};memcpy(packet,head,4);
 for(unsigned i=0;i<32;i++){
  uint8_t*p=packet+4+i*264;const uint8_t h[]={23,0,160,240,106,0,0,1};
  memcpy(p,h,8);for(unsigned j=0;j<256;j++)p[8+j]=(uint8_t)((i*256+j)*157+7);
 }
}
static int boundaries(void){
 static const unsigned lengths[]={1,15,16,17,255,256,257,1024};
 tm_source=zeros;tm_callback=completion;
 for(unsigned i=0;i<sizeof lengths/sizeof lengths[0];i++){
  unsigned previous=callbacks;tm_length=lengths[i];done=0;
  if(!start_write()||!wait_done()||callbacks!=previous+1||!finish())return 0;
 }
 unsigned previous=callbacks;tm_callback=0;tm_length=1;
 if(!start_write()||!finish()||callbacks!=previous)return 0;
 return record("Boundary lengths1/15/16/17/255/256/257/1024 and NULL callback PASS");
}
int main(int argc,char**argv){
 unsigned async=argc<2||atoi(argv[1])!=0;int ok=1,hook=0;
 putch(12);puts("EIRENE SINGLE-BUFFER UART FIXTURE\nAllow two minutes. A stall is FAIL; report the last progress line.\n");
 sv=mos_sysvars();if(!report_open())return 1;
 record("TM001 build=" TM_BUILD);record(async?"MODE async":"MODE standard");
 send(v_setup,sizeof v_setup);
 if(!gp(v_mismatch_probe,sizeof v_mismatch_probe,1)||!gp(v_match_probe,sizeof v_match_probe,0))ok=0;
 if(ok)ok=record("Comparator negative/positive PASS");
 if(async&&ok){
  tm_source=zeros;tm_callback=completion;tm_length=0;
  if(tm_write()!=2)ok=0;
  tm_source=zeros;tm_length=16;if(tm_masked_probe()!=3||!finish())ok=0;
  if(ok)ok=record("Zero-length/masked-context refusal PASS");
  if(ok)ok=boundaries();
  /* A valid input-only keyboard hook no longer excludes an async session.
     No synthetic key traffic is claimed by this fixture. */
  mos_setkbvector(key_hook,0);hook=1;
 }
 fill();const uint8_t clear[]={23,0,160,240,106,2};send(clear,sizeof clear);
 if(ok)ok=record("Transfer: GP prefix plus8192payloadbytes; checking concurrent RX");
 if(ok){
  unsigned previous=callbacks;sv[sysvar_gp]=0xfe;tm_source=packet;tm_length=sizeof packet;tm_callback=completion;done=0;
  if(async){
   if(!start_write())ok=0;
   else{
    if(tm_write()!=1)ok=0; /* no overwrite of the accepted large source */
    uint8_t early=0;uint32_t start=getsysvar_time();
    while(!done){if(sv[sysvar_gp]==0x51)early=1;if((uint32_t)(getsysvar_time()-start)>1000){ok=0;break;}}
    if(callbacks!=previous+1||!early)ok=0;
    if(ok)memset(packet,0xee,sizeof packet); /* Source is now reusable. */
   }
   if(!finish())for(;;){} /* Never return/free an uncertain accepted source. */
  }else send(packet,sizeof packet);
  if(async&&ok)ok=record("Single source/BUSY/callback/input-hook/RX-during-TX PASS");
  if(ok)ok=verify();
  if(ok)ok=record("VERIFY8192 PASS");
 }
 if(async){if(!finish())for(;;){};if(hook)mos_setkbvector(0,0);}
 if(ok){send(v_cleanup,sizeof v_cleanup);ok=record("TM001 MINIMAL PASS");}
 else record("TM001 MINIMAL FAIL");
 mos_fclose(file);return ok?0:1;
}
