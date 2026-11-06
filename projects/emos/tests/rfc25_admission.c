/* Execute the actual C admission; hardware state only is supplied by this model.
 * No new API, emulator image, target or instruction-timing claim. */
#include <assert.h>
#include <stdint.h>
#include "emos.h"
#include "emos_parallel_handover.h"
BYTE emos_rfc25_application=1,emos_rfc25_owned;
volatile BYTE emosVduBackend;
static BYTE context=1,mode,tx,boot=1;
static t_emosParallelHandover handover;
BYTE emos_application_context(void){return context;}
BYTE emos_get_mode(void){return mode;}
BYTE emos_uart0_async_owned(void){return tx;}
BYTE emos_parallel_boot_uart_allowed(void){return boot;}
t_emosParallelHandover*emos_parallel_boot_handover(void){return &handover;}
extern BYTE emos_rfc25_admit(UINT24,UINT24,UINT24);
static void reset(void){context=emos_rfc25_application=boot=1;mode=emosVduBackend=tx=emos_rfc25_owned=0;handover.phase=EPH_UART;}
static void rejected(UINT24 p,UINT24 n,UINT24 cb,BYTE result){
 assert(emos_rfc25_admit(p,n,cb)==result);assert(!emos_rfc25_owned);
}
int main(void){
 reset();emos_rfc25_application=0;rejected(0x40000,1,0x40100,32);
 reset();context=0;rejected(0x40000,1,0x40100,32);
 reset();rejected(0x3ffff,1,0x40100,19);rejected(0xb0000,1,0x40100,19);
 rejected(0x40000,0,0x40100,19);rejected(0xaffff,2,0x40100,19);
 rejected(0x40000,1,0x3ffff,19);rejected(0x40000,1,0xb0000,19);
 reset();mode=1;rejected(0x40000,1,0x40100,35);
 reset();emosVduBackend=1;rejected(0x40000,1,0x40100,35);
 reset();tx=1;rejected(0x40000,1,0x40100,31);
 reset();boot=0;rejected(0x40000,1,0x40100,31);
 reset();handover.phase=EPH_BLOCK;rejected(0x40000,1,0x40100,31);
 const UINT24 sizes[]={1,16,17,65535,65536,0x70000};
 for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i){
  reset();assert(emos_rfc25_admit(0x40000,sizes[i],0x40100)==0);assert(emos_rfc25_owned==1);
 }
 return 0;
}
