/* Adapter only: the frozen PERF1 timed C workload is unchanged. */
#include <agon/mos.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
extern const uint8_t *old_discover(void);
extern uint8_t old_call(void);
extern void old_notify(void);
void *old_function;
uint24_t old_hl,old_bc,old_de,old_result;
volatile uint8_t old_bad;
static const uint8_t *api;
static uint8_t opened,configured;
static uint8_t call(unsigned offset,uint24_t hl,uint24_t bc,uint24_t de){
 old_function=(void*)(api[offset]+((uint24_t)api[offset+1]<<8)+((uint24_t)api[offset+2]<<16));
 old_hl=hl;old_bc=bc;old_de=de;return old_call();
}
int legacy_init(void){
 api=old_discover();
 return api&&!memcmp(api,"UTX1",4)&&api[4]==1&&api[5]>=3&&api[6]>=69&&(api[7]&16)&&(api[53]&128);
}
uint8_t legacy_write(void){
 uint8_t rc;
 if(old_bad)return 2;
 if(!opened){
  rc=call(13,0,0,0);if(rc)return rc;opened=1;
 }
 if(!configured){
  rc=call(46,0,16u<<8,0);if(rc)return rc;
  /* Borrowed input, direct callback, no copied RST18 and no payload copy. */
  rc=call(54,(uint24_t)old_notify,1,0);if(rc)return rc;configured=1;
 }
 return call(66,(uint24_t)tm_source,tm_length,0);
}
uint8_t legacy_finish(void){
 if(!opened)return 0;
 uint8_t rc=call(31,0,0,0);if(rc)return rc;
 rc=call(34,0,0,0);if(rc)return rc;
 opened=configured=0;return old_bad?2:0;
}
int legacy_cleanup(void){
 uint32_t start=getsysvar_time();uint8_t rc;
 do{rc=legacy_finish();if(!opened)return 1;if(rc>1)return 0;}while((uint32_t)(getsysvar_time()-start)<1000);
 return 0;
}
