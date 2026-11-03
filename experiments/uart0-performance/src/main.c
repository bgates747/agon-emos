/* Controlled transport measurement; all receiver checks/logging are untimed. */
#include <agon/mos.h>
#include <ez80f92.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "packets.h"
#include "identity.h"
#define FRAMES 32
#define FRAME 264
#define WINDOW 213996UL /* ceil(8192 *1152000 /44100) */
static uint8_t packet[FRAMES*FRAME],checks[3840],file;
static volatile uint8_t done;
static volatile unsigned callbacks;
static volatile uint8_t *sv;
static uint32_t state,units,crc_table[256];
static unsigned mode;
static int log_ok=1,accepted;
static char line[240];
static void completion(void){callbacks++;done=1;}
static void send(const uint8_t*p,unsigned n){mos_puts((const char*)p,n,0);}
static int record(const char*s){
 puts(s);unsigned n=strlen(s);
 if(log_ok&&(mos_fwrite(file,(char*)s,n)!=n||mos_fwrite(file,"\r\n",2)!=2))log_ok=0;
 return log_ok;
}
#include "run_counter.inc"
static unsigned report_id;
static int report_open(void){
 unsigned n=first_report();if(!n)return 0;char name[16];
 for(;n<10000;n++){
  snprintf(name,sizeof name,"R%04u.log",n);uint8_t f=mos_fopen(name,FA_READ);
  if(f){mos_fclose(f);continue;}
  file=mos_fopen(name,FA_WRITE|FA_CREATE_NEW);if(!file)return 0;
  report_id=n;return save_next_report(n+1);
 }return 0;
}
static int journal(const char*status){
 uint8_t f=mos_fopen("runs.log",FA_WRITE|FA_OPEN_APPEND);if(!f)return 0;
 int n=snprintf(line,sizeof line,"R%04u %s build=%s mode=%u\r\n",report_id,status,TM_BUILD,mode);
 unsigned got=mos_fwrite(f,line,(unsigned)n);mos_fclose(f);return got==(unsigned)n;
}
static uint32_t installed_crc(unsigned n){
 for(unsigned i=0;i<256;i++){
  uint32_t c=i;for(unsigned j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320UL:0);crc_table[i]=c;
 }
 uint32_t c=0xffffffffUL;c=crc_table[(c^rom_byte0())&255]^(c>>8);
 const volatile uint8_t*p=(const volatile uint8_t*)1;
 for(unsigned i=1;i<n;i++)c=crc_table[(c^*p++)&255]^(c>>8);
 return c^0xffffffffUL;
}
static int gp(const uint8_t*p,unsigned n,uint8_t expected){
 sv[sysvar_gp]=0xfe;send(p,n);uint32_t t=getsysvar_time();
 while(sv[sysvar_gp]!=expected)if((uint32_t)(getsysvar_time()-t)>1000)return 0;
 return 1;
}
static int drain(void){
 uint32_t t=ticks();uint8_t rc;
 if(mode){
  do{rc=tm_finish();if(rc>1)return 0;if((uint32_t)(ticks()-t)>TICK_HZ*5)return 0;}while(rc);
 }else{
  while(!(IO(UART0_LSR)&0x40))if((uint32_t)(ticks()-t)>TICK_HZ*5)return 0;
 }
 accepted=0;return 1;
}
static int submit(const uint8_t*p,unsigned n){
 if(!mode){send(p,n);done=1;return 1;}
 tm_source=p;tm_length=n;tm_callback=completion;done=0;
 uint8_t rc;uint32_t t=ticks();
 do{rc=tm_write();if(rc>1||(uint32_t)(ticks()-t)>TICK_HZ*5)return 0;}while(rc);
 accepted=1;return 1;
}
/* Identical useful-work quantum, with a data-dependent final checksum. */
static void work(void){
 uint32_t x=state;
 for(unsigned i=0;i<16;i++){x^=x<<13;x^=x>>17;x^=x<<5;}
 state=x;units++;
}
static int verify(void){
 send(v_reset_flag,sizeof v_reset_flag);
 uint8_t f=mos_fopen("checks.bin",FA_READ);if(!f)return 0;
 for(unsigned b=0;b<32;b++){
  if(mos_fread(f,(char*)checks,sizeof checks)!=sizeof checks){mos_fclose(f);return 0;}
  for(unsigned j=0;j<sizeof checks;j+=60){send(checks+j,60);if(!gp(v_reply,sizeof v_reply,0)){mos_fclose(f);return 0;}}
  if((b&7)==7&&!record("Receiver verification quarter complete")){mos_fclose(f);return 0;}
 }
 mos_fclose(f);return record("VERIFY8192 PASS");
}
static void fill(void){
 const uint8_t h[]={23,0,160,240,106,0,0,1};
 for(unsigned i=0;i<FRAMES;i++){
  uint8_t*p=packet+i*FRAME;memcpy(p,h,sizeof h);
  for(unsigned j=0;j<256;j++)p[8+j]=(uint8_t)((i*256+j)*157+7);
 }
}
static int trial(unsigned repeat,unsigned paced){
 snprintf(line,sizeof line,"Repeat %u/3 %s: 8192 payload bytes, 8448 wire bytes",repeat,paced?"paced 44100 payload B/s":"bulk");
 if(!record(line))return 0;
 const uint8_t clear[]={23,0,160,240,106,2};send(clear,sizeof clear);
 /* A remote execution fence drains printed breadcrumbs before timing. */
 const uint8_t fence[]={23,0,128,0x73};if(!gp(fence,sizeof fence,0x73)||!drain())return 0;
 state=0x12345678UL;units=0;unsigned previous=callbacks,misses=0;
 uint32_t call_ticks=0,release=0,max_late=0,due=0,remainder=0;
 uint32_t wall_start=wall_ticks(),start=ticks();
 unsigned count=paced?FRAMES:1;
 for(unsigned i=0;i<count;i++){
  uint32_t now;
  if(paced)while((uint32_t)(ticks()-start)<due)work();
  now=ticks()-start;if(paced&&now>due&&now-due>max_late)max_late=now-due;
  uint32_t before=ticks();if(!submit(packet+(paced?i*FRAME:0),paced?FRAME:sizeof packet))return 0;
  call_ticks+=ticks()-before;
  while(!done){if(paced)work();if((uint32_t)(ticks()-start)>TICK_HZ*5)return 0;}
  release=ticks()-start;
  if(paced){
   /* Rational deadlines: exactly 256 payload bytes per 1/44100 second. */
   remainder+=256UL*TICK_HZ;due+=remainder/44100UL;remainder%=44100UL;
   if(release>due)misses++;
  }
 }
 /* Continue useful work to the exact observation window; no work in bulk. */
 if(paced)while((uint32_t)(ticks()-start)<WINDOW)work();
 if(!drain())return 0;
 uint32_t wire=ticks()-start,wall=wall_ticks()-wall_start;
 unsigned expected=mode?(paced?FRAMES:1):0;
 if(callbacks-previous!=expected)return 0;
 /* A separately reported remote fence is NOT in the local-drain metric. */
 uint32_t ft=ticks();if(!gp(fence,sizeof fence,0x73))return 0;ft=ticks()-ft;
 snprintf(line,sizeof line,"DATA rep=%u paced=%u ticks=%lu wall=%lu call=%lu source_observed=%lu fence=%lu work=%lu state=%08lX late=%lu missed=%u callbacks=%u",
  repeat,paced,(unsigned long)wire,(unsigned long)wall,(unsigned long)call_ticks,(unsigned long)release,(unsigned long)ft,(unsigned long)units,(unsigned long)state,(unsigned long)max_late,misses,callbacks-previous);
 if(!record(line)||!verify())return 0;
 return 1;
}
int main(int argc,char**argv){
 putch(12);puts("EIRENE UART PERFORMANCE COMPARISON\nAllow two minutes. A stall is FAIL; record the last progress line.\n");
 puts("RUN . 0=pristine Eirene, 1=scalar control, 2=OTIRX candidate. No firmware is flashed.");
 if(argc!=2||strlen(argv[1])!=1||argv[1][0]<'0'||argv[1][0]>'2')return 1;
 mode=(unsigned)(argv[1][0]-'0');sv=mos_sysvars();if(!report_open())return 1;
 if(!journal("START")){mos_fclose(file);return 1;}
 record("PERF1 build=" TM_BUILD);
 snprintf(line,sizeof line,"MODE %u; PRT1=1152000Hz PRT2=72000Hz payload=8192 wire=8448 window=%lu",mode,(unsigned long)WINDOW);record(line);
 puts("Checking installed MOS against the pinned image prefix...");
 uint32_t crc=installed_crc(mos_sizes[mode]);
 snprintf(line,sizeof line,"MOS bytes=%u CRC32=%08lX expected=%08lX",mos_sizes[mode],(unsigned long)crc,(unsigned long)mos_crcs[mode]);record(line);
 int ok=log_ok&&crc==mos_crcs[mode],clock=0;
 if(!ok)record("FAIL: installed MOS does not match selection; no experimental call made");
 if(ok){clock=clocks_start();ok=clock;if(!ok)record("FAIL: PRT1 or PRT2 already owned");}
 if(ok){
  /* Drain preflight output before establishing no-traffic work capacity. */
  const uint8_t barrier[]={23,0,128,0x74};
  if(!gp(barrier,sizeof barrier,0x74)){clocks_stop();record("PERF1 FAIL");journal("FAIL");mos_fclose(file);return 1;}
  /* Check two independent counters and establish no-traffic work capacity. */
  state=0x12345678UL;units=0;uint32_t w=wall_ticks(),t=ticks();
  while((uint32_t)(ticks()-t)<WINDOW)work();
  uint32_t elapsed=ticks()-t;w=wall_ticks()-w;
  snprintf(line,sizeof line,"CAL ticks=%lu wall=%lu work=%lu state=%08lX",(unsigned long)elapsed,(unsigned long)w,(unsigned long)units,(unsigned long)state);record(line);
  uint32_t projected=w*16UL,diff=elapsed>projected?elapsed-projected:projected-elapsed;
  if(!units||diff>elapsed/100+64){ok=0;record("FAIL: independent timer ratio disagrees by more than1percent");}
 }
 if(ok){
  fill();send(v_setup,sizeof v_setup);
  ok=gp(v_mismatch_probe,sizeof v_mismatch_probe,1)&&gp(v_match_probe,sizeof v_match_probe,0);
  if(ok)record("Comparator negative/positive PASS");
  for(unsigned r=1;r<=3&&ok;r++)for(unsigned p=0;p<2&&ok;p++)ok=trial(r,(r&1)?p:1-p);
 }
 if(accepted){
  /* Sources are static and retained. Never return with a live borrowed source. */
  /* If still live, even printf would compete for UART ownership. The last
     printed phase identifies this timeout; retain source/code until reset. */
  if(!drain())for(;;){}
 }
 if(clock)clocks_stop();
 if(ok){send(v_cleanup,sizeof v_cleanup);ok=record("PERF1 PASS");}else record("PERF1 FAIL");
 if(!journal(ok?"PASS":"FAIL"))ok=0;
 mos_fclose(file);return ok&&log_ok?0:1;
}
