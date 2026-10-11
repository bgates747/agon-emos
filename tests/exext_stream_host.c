/* Execute the actual batching/idle-service owner. Only physical/control peers
 * and the interrupt bridge are modeled; linked CPU checks cover bridge bytes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emos_exext.h"
#include "emos_parallel.h"
#include "emos_parallel_handover.h"
#include "emos_keyboard.h"
#include "uart.h"
volatile BYTE emos_console_owned=1,emosVduBackend,emos_key_faulted,keycount;
volatile BYTE hostPbDr=0x10;
static BYTE irq=1,reserved,in_tick,fail_block,parser_state;
static BYTE sent[16384], replies[1024], published[1024];
static unsigned sent_n,reply_n,published_n,blocks,fences;
static t_emosParallelHandover handover={EPH_UART,1,0,0};
extern void emos_exext_service(void);
static void tick(void);
BYTE emos_keyboard_clock(void){return 0;}
BYTE emos_keyboard_deadline_step(t_emosDeadline *d){tick();return ++d->elapsed<600;}
BYTE emos_keyboard_lock(void){BYTE old=irq;irq=0;return old;}
void emos_keyboard_unlock(BYTE on){irq=on;}
BYTE emos_parallel_boot_uart_allowed(void){return 2;}
t_emosParallelHandover *emos_parallel_boot_handover(void){return &handover;}
void emos_parallel_boot_block_end(BYTE ok){if(!ok)++fences;}
BYTE emos_keyboard_parallel_reserve(void){assert(irq);if(reserved||parser_state)return UART_POLL_UNAVAILABLE;reserved=1;return UART_POLL_READY;}
BYTE emos_keyboard_parallel_release(void){assert(irq&&reserved);reserved=0;return UART_POLL_READY;}
BYTE emos_parallel_session_negotiate(t_emosParallelHandover *h,t_parallelSession *s,const BYTE *transaction){
 assert(irq&&reserved&&h==&handover);s->phase=PARALLEL_SESSION_ACTIVE;memcpy(s->bytes,transaction,4);s->bytes[4]=s->bytes[5]=0;return 1;
}
BYTE emos_parallel_stream_control(t_parallelSession *s,BYTE op,UINT16 *n){
 assert(irq&&reserved&&s->phase==PARALLEL_SESSION_ACTIVE&&emos_exext_owned);
 assert(op==PARALLEL_STREAM_START||op==PARALLEL_STREAM_STOP||op==PARALLEL_STREAM_POLL);
 *n=reply_n;return 1;
}
BYTE emos_parallel_block_offer(t_emosParallelHandover *h,t_parallelSession *s,BYTE mode,const BYTE *offer){
 assert(irq&&reserved&&mode==PARALLEL_EXEXT&&h==&handover);
 assert(parallel_offer_valid(s->bytes,offer));s->bytes[4]=offer[8];s->bytes[5]=offer[9];return 1;
}
BYTE emos_parallel_native_coordinate(t_parallelSession *s,const BYTE *d,BYTE *p,UINT24 capacity){
 unsigned n=d[10]|((unsigned)d[11]<<8);(void)s;assert(irq&&reserved&&n==capacity);++blocks;
 if(fail_block)return 0;
 if(d[12]){assert(n<=reply_n);memcpy(p,replies,n);memmove(replies,replies+n,reply_n-n);reply_n-=n;}
 else{assert(sent_n+n<=sizeof(sent));memcpy(sent+sent_n,p,n);sent_n+=n;}
 reserved=0;return 1;
}
void emos_keyboard_byte(BYTE b){
 static BYTE command,left;
 assert(in_tick&&!irq);assert(published_n<sizeof(published));published[published_n++]=b;
 if(!parser_state){if(b&0x80){command=b;parser_state=1;}}
 else if(parser_state==1){left=b;parser_state=b?2:0;}
 else if(!--left){if(command==0x81)++keycount;parser_state=0;}
}
void emos_exext_interrupt_service(void){assert(in_tick&&!irq&&emos_exext_busy);irq=1;emos_exext_service();irq=0;}
static void tick(void){assert(irq);irq=0;in_tick=1;emos_exext_tick();assert(!irq);in_tick=0;irq=1;}
int main(void){
 BYTE data[4096];unsigned i;
 for(i=0;i<sizeof(data);++i)data[i]=(BYTE)(i*73u);
 assert(emos_exext_start());assert(emos_exext_owned&&!emos_exext_busy);
 emosVduBackend=2;
#ifdef EMOS_STREAM_TIMING
 /* Explicit drain uses the ordinary owner, including refusal semantics.
  * Empty drains emit no block; a residual drain neither loses nor replays data. */
 assert(emos_parallel_route_write_stream(data,0)==0);assert(!blocks);
 assert(emos_parallel_route_write_stream(data,17)==0);assert(!blocks);
 irq=0;assert(emos_parallel_route_write_stream(data,0)==EMOS_PARALLEL_BUSY);irq=1;
 assert(!blocks&&!sent_n);
 emos_exext_busy=1;assert(emos_parallel_route_write_stream(data,0)==EMOS_PARALLEL_BUSY);emos_exext_busy=0;
 emosVduBackend=1;assert(emos_parallel_route_write_stream(data,0)==EMOS_PARALLEL_BUSY);emosVduBackend=2;
 assert(emos_parallel_route_write_stream(data,0)==0);
 assert(blocks==1&&sent_n==17&&!memcmp(sent,data,17));
 assert(emos_parallel_route_write_stream(data,0)==0);assert(blocks==1&&sent_n==17);
 blocks=sent_n=0;
#endif
 assert(emos_parallel_route_write_stream(data,4095)==0);assert(!blocks&&!sent_n);
 assert(emos_parallel_route_write_byte(data[4095])==0);assert(blocks==1&&sent_n==4096&&!memcmp(sent,data,4096));
 assert(emos_parallel_route_write_stream(data,7)==0);assert(blocks==1);
 memcpy(replies,"\x81\x04\x61\x00\x01\x01",6);reply_n=6;
 // A selected stock SD transaction must not be preempted by physical blocks.
 hostPbDr=0;tick();assert(blocks==1&&sent_n==4096&&reply_n==6&&!published_n);
 hostPbDr=0x10;tick();assert(blocks==3&&sent_n==4103&&!memcmp(sent+4096,data,7));
 assert(published_n==6&&!memcmp(published,"\x81\x04\x61\x00\x01\x01",6));
 // No application output: a second physical key/reply still reaches the ISR.
 memcpy(replies,"\x81\x04\x00\x00\x01\x00",6);reply_n=6;tick();assert(published_n==12&&blocks==4);
 // Bundled down/up must expose each event in a separate real interrupt.
 // Other replies retain order; unpublished bytes cannot be overwritten.
 {const BYTE batch[]={0x83,1,0x52,0x81,4,'b',0,2,1,0x81,4,0,0,2,0,0x83,1,0x62};
  memcpy(replies,batch,sizeof(batch));reply_n=sizeof(batch);
  tick();assert(keycount==3&&published_n==21&&blocks==5);
  hostPbDr=0; // Already received packets still publish during SD selection.
  tick();assert(keycount==4&&published_n==27&&blocks==5);
  tick();assert(keycount==4&&published_n==30&&blocks==5);
  assert(!memcmp(published+12,batch,sizeof(batch)));
  hostPbDr=0x10;
 }
 emos_exext_busy=1;assert(emos_parallel_route_write_byte(4)==EMOS_PARALLEL_BUSY);tick();assert(blocks==5);emos_exext_busy=0;
 irq=0;assert(emos_parallel_route_write_byte(4)==EMOS_PARALLEL_BUSY);assert(!irq);irq=1;
 memcpy(replies,"\x81\x04\x00\x00\x01\x00",6);reply_n=6;
 assert(emos_exext_stop());assert(!emos_exext_owned&&!emos_exext_busy);tick();assert(blocks==6&&published_n==36);
 assert(emos_exext_start());assert(emos_parallel_route_write_byte(9)==0);
 fail_block=1;tick();assert(fences&&!emos_exext_owned);assert(emos_parallel_route_write_byte(1)==EMOS_PARALLEL_BUSY);
 // Actual reservation requires the UART parser at a packet boundary. Exercise
 // all header offsets around the256-byte return cut, including a257-byte
 // unknown packet. High-bit body bytes must never become new headers. STOP
 // must fetch an incomplete tail instead of waiting forever for publication.
 fail_block=0;reserved=0; // Model physical reset recovery after the fault fence.
 {const unsigned lengths[]={0,1,4,16,127,239,240,255};unsigned cases=0;
  for(unsigned stopping=0;stopping<2;++stopping)
  for(unsigned j=0;j<sizeof(lengths)/sizeof(*lengths);++j)
  for(unsigned offset=0;offset<256;++offset){
   BYTE expected[1024];unsigned n=offset+2+lengths[j]+6;
   assert(emos_exext_start());published_n=0;
   memset(replies,0x2a,offset);replies[offset]=0x8e;replies[offset+1]=lengths[j];
   for(unsigned k=0;k<lengths[j];++k)replies[offset+2+k]=(BYTE)(0x80+k);
   memcpy(replies+offset+2+lengths[j],"\x81\x04\x61\x00\x01\x00",6);
   memcpy(expected,replies,n);reply_n=n;tick();
   if(!stopping)for(unsigned k=0;k<8 && (reply_n||published_n<n);++k)tick();
   assert(emos_exext_stop());
   assert(!parser_state && !reply_n && published_n==n && !memcmp(published,expected,n));++cases;
  }
  printf("PASS %u split-return/header/body/STOP cases with actual parser-boundary reservation\n",cases);
 }
 /* Byte-only and mixed callers must share exactly one boundary/ordering. */
 sent_n=0;blocks=0;assert(emos_exext_start());
 for(i=0;i<4096;++i)assert(emos_parallel_route_write_byte(data[i])==0);
 assert(blocks==1&&sent_n==4096&&!memcmp(sent,data,4096));
 assert(emos_parallel_route_write_stream(data,4094)==0);
 assert(emos_parallel_route_write_byte(data[4094])==0);
 assert(emos_parallel_route_write_byte(data[4095])==0);
 assert(blocks==2&&sent_n==8192&&!memcmp(sent+4096,data,4096));
 assert(emos_exext_stop());
 puts("PASS actual EMOS byte batching, full-block flush, idle replies/keys, IRQ context, nested refusal and fault fence");
 return 0;
}
