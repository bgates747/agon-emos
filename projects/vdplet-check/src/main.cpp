// INTEG-015 v0.1.27 support: finite console-only VDPL admission/Escape check.
// No rendering benchmark, mode switch, timer writes, UART access or SD writes.
// The caller SAVE's the exported result before loading another application.
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern "C" {
volatile uint8_t vd27_reply[10],vd27_ready,vd27_escape;
uint8_t vd27_result[64];
void vd27_callback(void);
}
static uint32_t clock_now(){
 uint32_t a,b;
 do{a=getsysvar_time();b=getsysvar_time();}while(a!=b);
 return a;
}
static void send(const uint8_t *p,uint8_t n){mos_puts((const char *)p,n,0);}
static bool wait_flag(volatile uint8_t *p,uint8_t mask,uint32_t ticks){
 uint32_t began=clock_now();uint24_t budget=1000000;
 while(!(*p&mask)&&budget--){if(uint32_t(clock_now()-began)>ticks)return false;}
 return (*p&mask)!=0;
}
int main(){
 memset(vd27_result,0,sizeof vd27_result);memcpy(vd27_result,"VD27",4);
 vd27_ready=vd27_escape=0;
 printf("Admission utility %s\r\nConsole discovery only; no benchmark\r\n",PROBE_BUILD_ID);
 volatile uint8_t *sv=mos_sysvars();
 sv[sysvar_vdp_pflags]&=uint8_t(~vdp_pflag_mode);
 const uint8_t mode[]={23,0,134};send(mode,sizeof mode);
 if(!wait_flag(sv+sysvar_vdp_pflags,vdp_pflag_mode,400))return 1;
 vd27_result[4]|=1;vd27_result[5]=sv[sysvar_scrMode];
 // Allow the invoking Enter release to finish before testing key isolation.
 uint32_t start=clock_now();uint24_t budget=1000000;
 while(uint32_t(clock_now()-start)<20&&budget--){}
 if(uint32_t(clock_now()-start)<20)return 1;
 volatile uint8_t *map=mos_getkbmap();
 for(uint8_t i=0;i<16;++i)vd27_result[32+i]=map[i];
 vd27_result[6]=sv[sysvar_vkeycount];
 mos_setkbvector(vd27_callback,0);
 const uint8_t discover[]={23,0,217};send(discover,sizeof discover);
 bool reply=wait_flag(&vd27_ready,1,500);
 vd27_result[7]=sv[sysvar_vkeycount];
 for(uint8_t i=0;i<10;++i)vd27_result[16+i]=vd27_reply[i];
 for(uint8_t i=0;i<16;++i)vd27_result[48+i]=map[i];
 if(reply&&!memcmp(vd27_result+16,"VDPL",4)&&vd27_reply[4]==1&&
    vd27_reply[5]==1&&vd27_reply[6]==0&&vd27_reply[7]==0&&(vd27_reply[8]&1))
  vd27_result[4]|=2;
 if(vd27_result[6]==vd27_result[7]&&!memcmp(vd27_result+32,vd27_result+48,16))
  vd27_result[4]|=4;
 if((vd27_result[4]&7)==7){
  printf("Discovery received; waiting for Escape\r\n");
  if(wait_flag(&vd27_escape,1,1200))vd27_result[4]|=8;
 }
 vd27_result[8]=sv[sysvar_vkeycount];
 mos_setkbvector(0,0);vd27_result[4]|=16;
 printf("Admission check ended: %s\r\n",vd27_result[4]==31?"PASS":"FAIL");
 return vd27_result[4]==31?0:1;
}
