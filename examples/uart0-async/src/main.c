/* Original AgonDev example: callback posts state; foreground does work. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include "uart0_async.h"
#include "identity.h"
#include "rom_crc.h"
static volatile uint8_t released;
static void complete(void){released=1;}
static uint32_t state=0x12345678UL,work_count;
static void useful_work(void){state^=state<<13;state^=state>>17;state^=state<<5;work_count++;}
int main(void){
 if(installed_crc(RC_MOS_BYTES)!=RC_MOS_CRC){puts("Wrong MOS for RC1 example; no async call made.");return 1;}
 static const char text[]="Asynchronous VDP output from the RC1 C example.\r\n";
 released=0; /* MUST precede submission, even with a short source. */
 uint8_t rc;
 do{rc=u0_submit(text,sizeof text-1,complete);}while(rc==U0_BUSY);
 if(rc!=U0_OK)return 1; /* No source accepted on an error. */
 while(!released)useful_work();
 /* Source consumed is not wire drain: finish before any ordinary output. */
 while((rc=u0_finish())==U0_BUSY)useful_work();
 if(rc!=U0_OK)for(;;){} /* Preserve source/callback storage on uncertain teardown. */
 printf("RC1 EXAMPLE PASS; work=%lu state=%08lX\n",(unsigned long)work_count,(unsigned long)state);
 return 0;
}
