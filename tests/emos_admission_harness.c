#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emos_admission.h"
#include "emos_keyboard.h"
#include "emos.h"
volatile BYTE keycount, emos_key_source=2, emos_key_faulted, uart1_keyboard_owned=1;
static BYTE irq=1,tick,mode,core=1,send_fail;
static BYTE sent[52];
static unsigned sends,runs;
static int run_result=4;
BYTE emos_keyboard_lock(void){BYTE old=irq;irq=0;return old;}
void emos_keyboard_unlock(BYTE b){irq=b;}
BYTE emos_keyboard_clock(void){return tick;}
BYTE emos_get_mode(void){return mode;}
BYTE emos_admission_core(void){return core;}
BYTE emos_keyboard_send(const BYTE *p,UINT16 n){assert(irq&&n==52);memcpy(sent,p,n);++sends;return send_fail;}
int emos_admission_run(void){assert(irq && emos_admission_dispatching());++runs;return run_result;}
/* Include maintained implementation to inject wrap/clock fault boundaries. */
#include "../src/emos_admission.c"
static void answer(BYTE status, BYTE is_job) {
    BYTE p[48];memcpy(p,sent+4,48);p[3]=5;p[13]=status;
    if(p[12]==HELLO){p[20]=0x53;p[46]=7;}
    if(is_job){p[32]=17;p[44]=1;p[45]=3;}
    put32(p+16,crc(p));emos_admission_packet(p,48);
}
static void fresh(void){
    emos_admission_reset();tick++;retrying=0;mode=0;mode_seen=0;core=1;send_fail=0;
    assert(!emos_admission_idle(keycount));assert(sent[16]==HELLO);
    answer(0,0);assert(!emos_admission_idle(keycount));
    tick++;assert(!emos_admission_idle(keycount));assert(sent[16]==POLL);
}
int main(void){
    /* Empty prompt handshake, offer alone does not launch. */
    fresh();answer(0,1);assert(!emos_admission_idle(keycount));
    assert(sent[16]==DECIDE && !runs);
    answer(0,0);assert(emos_admission_idle(keycount));
    assert(emos_admission_dispatch()==4 && runs==1 && sent[16]==CLOSE);
    assert(!emos_admission_dispatching());
    assert(emos_admission_dispatch()==EMOS_BUSY && runs==1);
    /* Late key wins even after peer acknowledged provisional grant. */
    fresh();answer(0,1);assert(!emos_admission_idle(keycount));answer(0,0);
    {BYTE observed=keycount;keycount++;assert(!emos_admission_idle(observed));}
    assert(emos_admission_dispatch()==EMOS_BUSY && runs==1);
    /* Partial CLI/app transition retires offer; no replay at later prompt. */
    fresh();answer(0,1);emos_admission_leave();tick++;
    assert(!emos_admission_idle(keycount));assert(sent[16]==POLL);
    core=0;answer(0,1);assert(!emos_admission_idle(keycount));assert(runs==1);
    /* Wrong CRC/sequence/incarnation cannot become permission. */
    fresh();answer(0,1);rx[16]^=1;assert(!emos_admission_idle(keycount));
    answer(0,1);rx[8]++;put32(rx+16,crc(rx));assert(!emos_admission_idle(keycount));
    answer(0,1);rx[20]++;put32(rx+16,crc(rx));assert(!emos_admission_idle(keycount));
    assert(emos_admission_dispatch()==EMOS_BUSY);
    /* Stalled peer and stalled clock are both bounded, no tight re-probes. */
    tick+=24;assert(!emos_admission_idle(keycount));
    {unsigned n=sends;tick++;assert(!emos_admission_idle(keycount));assert(sends==n);}
    fresh();spin_budget=1;assert(!emos_admission_idle(keycount));assert(state==OFF);
    /* No bootstrap into active VDU parser; negotiated mode allowed. */
    emos_admission_reset();mode=1;tick++;{unsigned n=sends;assert(!emos_admission_idle(keycount));assert(sends==n);}
    fresh();emos_admission_leave();mode=1;tick++;assert(!emos_admission_idle(keycount));assert(sent[16]==POLL);
    /* Loss of input path discards grants. */
    emos_key_faulted=1;assert(!emos_admission_idle(keycount));assert(state==OFF);emos_key_faulted=0;
    fresh();answer(0,1);assert(!emos_admission_idle(keycount));answer(0,0);assert(emos_admission_idle(keycount));
    run_result=0;assert(emos_admission_dispatch()==EMOS_UNAVAILABLE);assert(sent[17]==5);
    fresh();sequence=0xffffffffUL;tick+=24;retrying=0;state=OFF;tick++;
    assert(!emos_admission_idle(keycount));assert(exhausted);
    assert(irq);
    puts("admission: idle, stale/key races, CRC, timeouts, mode gate, loader failure and no false success pass");
}
