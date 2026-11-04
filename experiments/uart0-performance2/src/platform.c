/* PRT snapshot method from the qualified original balls application. */
#include <agon/mos.h>
#include <ez80f92.h>
#include "platform.h"
const uint8_t *tm_source;
uint24_t tm_length;
void (*tm_callback)(void);
volatile uint24_t perf_timer1_wraps,perf_timer2_wraps;
volatile uint24_t perf_clock_wrap,perf_clock_count,perf_wall_wrap,perf_wall_count;
extern void perf_timer1_isr(void),perf_timer2_isr(void);
extern void perf_capture_clock(void),perf_capture_wall(void);
static void *old1,*old2;
static uint8_t ctl1,ctl2,iss,running;
int clocks_start(void){
 ctl1=IO(TMR1_CTL);ctl2=IO(TMR2_CTL);
 if((ctl1|ctl2)&1)return 0;
 iss=IO(TMR_ISS);perf_timer1_wraps=perf_timer2_wraps=0;
 old1=mos_setintvector(0x0c,perf_timer1_isr);old2=mos_setintvector(0x0e,perf_timer2_isr);
 IO(TMR1_CTL)=0;IO(TMR2_CTL)=0;IO(TMR_ISS)=iss&0xc3;
 IO(TMR1_RR_L)=0;IO(TMR1_RR_H)=0;IO(TMR2_RR_L)=0;IO(TMR2_RR_H)=0;
 IO(TMR1_CTL)=0x57;IO(TMR2_CTL)=0x5f;running=1;return 1;
}
void clocks_stop(void){
 if(!running)return;
 IO(TMR1_CTL)=0;IO(TMR2_CTL)=0;
 mos_setintvector(0x0c,old1);mos_setintvector(0x0e,old2);
 IO(TMR_ISS)=iss;IO(TMR1_CTL)=ctl1&0x7e;IO(TMR2_CTL)=ctl2&0x7e;running=0;
 /* Only disabled timers were claimed. Reload registers are write-only;
    later owners configure their reloads, as for normal PRT ownership. */
}
uint32_t ticks(void){perf_capture_clock();return ((uint32_t)perf_clock_wrap<<16)+(uint16_t)(0-(uint16_t)perf_clock_count);}
uint32_t wall_ticks(void){perf_capture_wall();return ((uint32_t)perf_wall_wrap<<16)+(uint16_t)(0-(uint16_t)perf_wall_count);}
