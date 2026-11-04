/* Original C/AgonDev benchmark. See task bulk-work.md for measurement limits. */
#include <agon/mos.h>
#include <ez80f92.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "config.h"
#include "packets.h"
#define FRAMES 32
#define FRAME 264
#define WINDOW 213996UL
#define BULK 0
#define PACED 1
#define BULK_WORK 2
static uint8_t packet[FRAMES*FRAME],checks[3840],file;
static volatile uint8_t done;
static volatile unsigned callbacks;
static volatile uint8_t *sv;
static uint32_t state,units,crc_table[256];
static unsigned report_id;
static int log_ok=1,accepted;
static char line[320];
static const char *scenario_name[]={"bulk only","paced plus calculation","bulk plus calculation"};
static void completion(void){callbacks++;done=1;}
static void send(const uint8_t*p,unsigned n){mos_puts((const char*)p,n,0);}
static int record(const char*s){
 puts(s);unsigned n=strlen(s);
 if(log_ok&&(mos_fwrite(file,(char*)s,n)!=n||mos_fwrite(file,"\r\n",2)!=2))log_ok=0;
 return log_ok;
}
#include "run_counter.inc"
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
 int n=snprintf(line,sizeof line,"R%04u %s PERF2 build=%s case=%s\r\n",report_id,status,fixture.build,fixture.id);
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
static uint8_t drain_probe(void){
 if(fixture.transport)return tm_finish();
 return (IO(UART0_LSR)&0x40)?0:1;
}
/* One step, with final state validated independently. Never reset by an ISR. */
static void work(void){
 uint32_t x=state;x^=x<<13;x^=x>>17;x^=x<<5;state=x;units++;
}
static int drain(unsigned useful){
 uint32_t t=ticks();uint8_t rc;
 while((rc=drain_probe())!=0){
  if(rc>1||(uint32_t)(ticks()-t)>TICK_HZ*5)return 0;
  if(useful)work();
 }
 accepted=0;return 1;
}
static int submit(const uint8_t*p,unsigned n){
 if(!fixture.transport){send(p,n);done=1;return 1;}
 tm_source=p;tm_length=n;tm_callback=completion;done=0;
 uint8_t rc;uint32_t t=ticks();
 do{rc=tm_write();if(rc>1||(uint32_t)(ticks()-t)>TICK_HZ*5)return 0;}while(rc);
 accepted=1;return 1;
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
static int trial(unsigned repeat,unsigned scenario){
 unsigned paced=scenario==PACED,useful=scenario!=BULK;
 snprintf(line,sizeof line,"Repeat %u/3: %s; 8192 payload / 8448 wire bytes",repeat,scenario_name[scenario]);
 if(!record(line))return 0;
 const uint8_t clear[]={23,0,160,240,106,2};send(clear,sizeof clear);
 const uint8_t fence[]={23,0,128,0x73};if(!gp(fence,sizeof fence,0x73)||!drain(0))return 0;
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
  while(!done){if(useful)work();if((uint32_t)(ticks()-start)>TICK_HZ*5)return 0;}
  release=ticks()-start;
  if(paced){
   remainder+=256UL*TICK_HZ;due+=remainder/44100UL;remainder%=44100UL;
   if(release>due)misses++;
  }
 }
 if(paced)while((uint32_t)(ticks()-start)<WINDOW)work();
 /* Bulk-work continues through TEMT, not merely source/callback completion. */
 if(!drain(scenario==BULK_WORK))return 0;
 uint32_t wire=ticks()-start,wall=wall_ticks()-wall_start;
 unsigned expected=fixture.transport?(paced?FRAMES:1):0;
 if(callbacks-previous!=expected)return 0;
 uint32_t ft=ticks();if(!gp(fence,sizeof fence,0x73))return 0;ft=ticks()-ft;
 /* Last bulk-work quantum may straddle actual wire completion. Discard it
    for conservative credited work; retain raw count/state for validation. */
 uint32_t credited=(scenario==BULK_WORK&&units)?units-1:units;
 snprintf(line,sizeof line,"DATA rep=%u scenario=%u ticks=%lu wall=%lu call=%lu source_observed=%lu fence=%lu work=%lu credited=%lu state=%08lX late=%lu missed=%u callbacks=%u",
  repeat,scenario,(unsigned long)wire,(unsigned long)wall,(unsigned long)call_ticks,(unsigned long)release,(unsigned long)ft,(unsigned long)units,(unsigned long)credited,(unsigned long)state,(unsigned long)max_late,misses,callbacks-previous);
 return record(line)&&verify();
}
int main(void){
 putch(12);printf("UART PERFORMANCE SUITE - %s\n",fixture.name);
 puts("Runs all 3 scenarios, 3 repeats each. Allow two minutes.\nA stall is FAIL; record the last progress line. No firmware is flashed.");
 sv=mos_sysvars();if(!report_open())return 1;
 if(!journal("START")){mos_fclose(file);return 1;}
 snprintf(line,sizeof line,"PERF2 build=%s case=%s",fixture.build,fixture.id);record(line);
 snprintf(line,sizeof line,"CONFIG transport=%u tick_hz=1152000 wall_hz=72000 payload=8192 wire=8448 window=%lu work_steps=1",fixture.transport,(unsigned long)WINDOW);record(line);
 puts("Checking installed MOS against this fixture...");
 uint32_t crc=installed_crc(fixture.mos_bytes);
 snprintf(line,sizeof line,"MOS bytes=%u CRC32=%08lX expected=%08lX",fixture.mos_bytes,(unsigned long)crc,(unsigned long)fixture.mos_crc);record(line);
 int ok=log_ok&&crc==fixture.mos_crc,clock=0;
 if(!ok)record("FAIL: wrong installed MOS; no experimental call made");
 if(ok&&fixture.transport==2&&!legacy_init()){ok=0;record("FAIL: previous UTX1 interface absent");}
 if(ok){clock=clocks_start();ok=clock;if(!ok)record("FAIL: PRT1 or PRT2 already owned");}
 if(ok){
  const uint8_t barrier[]={23,0,128,0x74};
  ok=gp(barrier,sizeof barrier,0x74);
  if(ok){
   state=0x12345678UL;units=0;uint32_t w=wall_ticks(),t=ticks();
   while((uint32_t)(ticks()-t)<WINDOW)work();
   uint32_t elapsed=ticks()-t;w=wall_ticks()-w;
   snprintf(line,sizeof line,"CAL ticks=%lu wall=%lu work=%lu state=%08lX",(unsigned long)elapsed,(unsigned long)w,(unsigned long)units,(unsigned long)state);record(line);
   uint32_t projected=w*16UL,diff=elapsed>projected?elapsed-projected:projected-elapsed;
   if(!units||diff>elapsed/100+64){ok=0;record("FAIL: timer counters disagree");}
  }
 }
 if(ok){
  fill();send(v_setup,sizeof v_setup);
  ok=gp(v_mismatch_probe,sizeof v_mismatch_probe,1)&&gp(v_match_probe,sizeof v_match_probe,0);
  if(ok)record("Comparator negative/positive PASS");
  for(unsigned r=1;r<=3&&ok;r++)for(unsigned s=0;s<3&&ok;s++)ok=trial(r,(r-1+s)%3);
 }
 if(accepted&&!drain(0))for(;;){} /* Retain source/code; never print into a live UART owner. */
 if(fixture.transport==2&&!legacy_cleanup())for(;;){}
 if(clock)clocks_stop();
 if(ok){send(v_cleanup,sizeof v_cleanup);ok=record("PERF2 PASS");}else record("PERF2 FAIL");
 if(!journal(ok?"PASS":"FAIL"))ok=0;
 mos_fclose(file);return ok&&log_ok?0:1;
}
