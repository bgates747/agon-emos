/* REMOTE-005 A04: one bounded foreground admission exchange, no file engine.
 * Core RAM holds replies; the IRQ only copies. Public mos_EDITLINE never calls
 * this code. Legacy HELLO is safe on old P4 (whole F6 consumed, kind rejected).
 * ExCom bootstrap is intentionally forbidden until a Legacy capability grant.
 * The separate finite sdjob utility is NOT the existing indefinite sdserve.
 * READY/FINISH belong to the finite utility, not resident polling. The private
 * handoff returns its final control sequence before resident CLOSE. Missing
 * utility, an unreported terminal result and old peers fail closed.
 */
#include <string.h>
#include "emos.h"
#include "emos_keyboard.h"
#include "emos_admission.h"
#include "uart.h"

extern volatile BYTE keycount;
enum { OFF, HELLO_WAIT, IDLE, POLL_WAIT, DECIDE_WAIT, CLAIMED, RUNNING };
enum { HELLO=1, POLL=2, DECIDE=3, CLOSE=10 };
static BYTE state, caps, mode_seen, last_tick, sent_at, retry_at, retrying;
static BYTE tx[52], rx[48], link[8], grant[8], job[4];
static volatile BYTE received, waiting;
static UINT32 sequence, generation=1, grant_counter, spin_budget;
static BYTE exhausted, job_class, utility_terminal;

static UINT32 u32(const BYTE *p) {
    return (UINT32)p[0] | ((UINT32)p[1]<<8) | ((UINT32)p[2]<<16) | ((UINT32)p[3]<<24);
}
static void put32(BYTE *p, UINT32 n) {
    p[0]=n; p[1]=n>>8; p[2]=n>>16; p[3]=n>>24;
}
static UINT32 crc(const BYTE *p) {
    UINT32 c=0xffffffffUL;
    BYTE i,b;
    for(i=0;i<48;++i) {
        if(i>=16 && i<20)continue;
        c^=p[i];
        for(b=0;b<8;++b)c=(c>>1)^((c&1)?0xedb88320UL:0);
    }
    return c^0xffffffffUL;
}
static BYTE nonzero(const BYTE *p, BYTE n) {
    BYTE v=0; while(n--)v|=*p++; return v!=0;
}
void emos_admission_packet(const BYTE *p, BYTE n) {
    /* ISR: bounded copy only; CRC/identity checks and decisions are foreground. */
    /* CLOSE is fire-and-forget. Its delayed ACK must not occupy the sole
     * reply slot after a new HELLO starts; retain CRC checks in foreground. */
    if(waiting && !received && n==48 && p[3]==5 && !memcmp(p+4,tx+8,9)) {
        memcpy(rx,p,48); received=1;
    }
}
static void clear_wait(void) {
    BYTE irq=emos_keyboard_lock(); waiting=received=0; emos_keyboard_unlock(irq);
}
void emos_admission_reset(void) {
    clear_wait(); state=OFF; caps=0; job_class=0; utility_terminal=0;
    memset(link,0,8); memset(job,0,4); memset(grant,0,8);
    /* Saturation disables admission rather than reusing an in-boot identity. */
    if(++generation==0)exhausted=1;
}
static BYTE send_control(BYTE op, BYTE result);
void emos_admission_leave(void) {
    /* Called before a key is interpreted and before application dispatch. */
    /* DECIDE is only a reservation, never permission for filesystem bytes.
     * Withdraw it when a key wins. Peer timeout covers a lost CLOSE. */
    if(state==DECIDE_WAIT)send_control(CLOSE,9);
    clear_wait(); state=caps ? IDLE : OFF;
    memset(job,0,4);memset(grant,0,8);job_class=0;
    if(++generation==0)exhausted=1;
}
BYTE emos_admission_dispatching(void) { return state==RUNNING; }
BYTE emos_admission_excom(void) { return (caps&7)==7; }
BYTE emos_admission_application(void) { return (caps&11)==11; }
static BYTE send_control(BYTE op, BYTE result) {
    BYTE *p=tx+4, irq;
    if(++sequence==0){exhausted=1;return 0;}
    memset(tx,0,sizeof(tx)); tx[0]=23;tx[2]=0xf6;tx[3]=48;
    p[0]='S';p[1]='D';p[2]=1;p[3]=4;
    put32(p+4,1);put32(p+8,sequence);p[12]=op;p[13]=result;p[14]=28;
    if(op==HELLO)p[46]=1;
    else {
        memcpy(p+20,link,8);put32(p+28,generation);memcpy(p+32,job,4);
        memcpy(p+36,grant,8);p[44]=nonzero(job,4)?1:0;p[45]=job_class;
    }
    put32(p+16,crc(p));
    irq=emos_keyboard_lock();received=0;waiting=1;emos_keyboard_unlock(irq);
    sent_at=emos_keyboard_clock();spin_budget=262144UL;
    if(emos_keyboard_send(tx,sizeof(tx))!=EMOS_KEY_OK) {
        emos_admission_reset();retrying=1;retry_at=sent_at;return 0;
    }
    return 1;
}
static BYTE reply(void) {
    BYTE irq, have;
    irq=emos_keyboard_lock();have=received;
    if(have)waiting=0;
    emos_keyboard_unlock(irq);
    if(!have)return 0;
    if(rx[0]!='S'||rx[1]!='D'||rx[2]!=1||rx[3]!=5||rx[14]!=28||rx[15]||
       u32(rx+16)!=crc(rx)||memcmp(rx+4,tx+8,9)) {
        clear_wait();waiting=1;return 0;
    }
    if(state!=HELLO_WAIT && (memcmp(rx+20,tx+24,26)||rx[46]||rx[47])) {
        /* POLL supplies a new job ID; other identity fields still must match. */
        if(state!=POLL_WAIT||memcmp(rx+20,link,8)||u32(rx+28)!=generation||
           nonzero(rx+36,8)||rx[46]||rx[47]) {clear_wait();waiting=1;return 0;}
    }
    received=0;return 1;
}
BYTE emos_admission_idle(BYTE observed) {
    BYTE now=emos_keyboard_clock(), irq, mode=emos_get_mode();
    if(exhausted || !emos_admission_core() || emos_key_source!=EMOS_KEY_EXTENDER ||
       emos_key_faulted || !uart1_keyboard_owned) {
        if(state!=OFF || caps)emos_admission_reset();
        return 0;
    }
    if(observed!=keycount){emos_admission_leave();return 0;}
    if(mode!=mode_seen) {emos_admission_leave();mode_seen=mode;}
    if(mode!=EMOS_MODE_LEGACY && !(caps&2))return 0;
    if(state==HELLO_WAIT || state==POLL_WAIT || state==DECIDE_WAIT) {
        if(reply()) {
            if(state==HELLO_WAIT) {
                if(rx[13]||!nonzero(rx+20,8)||nonzero(rx+28,18)||rx[47]||
                   (rx[46]&5)!=5 || (rx[46]&0xf0)) {
                    emos_admission_reset();retrying=1;retry_at=now;return 0;
                }
                memcpy(link,rx+20,8);caps=rx[46];state=IDLE;
            } else if(state==POLL_WAIT) {
                if(rx[13]==1 && !nonzero(rx+32,4) && !rx[44] && !rx[45])state=IDLE;
                else if(!rx[13] && nonzero(rx+32,4) && rx[44]==1 && rx[45]>=1 && rx[45]<=8) {
                    /* Key arriving during CRC/reply handling must win. Claim is
                     * provisional until matching DECIDE acknowledgement. */
                    irq=emos_keyboard_lock();
                    if(observed!=keycount){emos_keyboard_unlock(irq);emos_admission_leave();return 0;}
                    memcpy(job,rx+32,4);job_class=rx[45];
                    if(++grant_counter==0){exhausted=1;emos_keyboard_unlock(irq);emos_admission_reset();return 0;}
                    put32(grant,grant_counter);put32(grant+4,generation);
                    state=DECIDE_WAIT;emos_keyboard_unlock(irq);
                    send_control(DECIDE,0);return 0;
                } else {emos_admission_reset();return 0;}
            } else {
                if(rx[13]){emos_admission_reset();return 0;}
                irq=emos_keyboard_lock();
                if(observed!=keycount){emos_keyboard_unlock(irq);emos_admission_leave();return 0;}
                state=CLAIMED;emos_keyboard_unlock(irq);return 1;
            }
        } else if((BYTE)(now-sent_at)>=24 || !--spin_budget) {
            emos_admission_reset();retrying=1;retry_at=now;return 0;
        } else return 0;
    }
    if(now==last_tick)return 0;
    last_tick=now;
    if(state==OFF) {
        if(mode!=EMOS_MODE_LEGACY || (retrying && (BYTE)(now-retry_at)<120))return 0;
        retrying=0;state=HELLO_WAIT;send_control(HELLO,0);
    } else if(state==IDLE) {state=POLL_WAIT;send_control(POLL,0);}
    return 0;
}
BYTE emos_admission_binding(BYTE *out) {
    if(state!=RUNNING || utility_terminal)return 0;
    memset(out,0,36);memcpy(out,link,8);put32(out+8,generation);
    memcpy(out+12,job,4);memcpy(out+16,grant,8);out[24]=1;out[25]=job_class;
    put32(out+28,1);put32(out+32,sequence);return 1;
}
BYTE emos_admission_terminal(const BYTE *p) {
    UINT32 n=u32(p);
    if(state!=RUNNING || utility_terminal || n<sequence || n==0xffffffffUL || p[4]>1)return 0;
    sequence=n;utility_terminal=p[4]?2:1;return 1;
}
int emos_admission_dispatch(void) {
    int result;
    if(state!=CLAIMED || !emos_admission_core())return EMOS_BUSY;
    state=RUNNING;utility_terminal=0;
    result=emos_admission_run();
    /* Only the claimed finite utility can acknowledge FINISH through sdlink.
     * A random MOSlet return never means the filesystem job completed. */
    if(!result && utility_terminal!=1)result=EMOS_UNAVAILABLE;
    if(!send_control(CLOSE,result ? 7 : 0) && !result)result=EMOS_UNAVAILABLE;
    /* ExCom cannot bootstrap HELLO. Retain the negotiated incarnation after
     * successful completion, but retire the job/grant. Faults invalidate it
     * and require return to Legacy for renegotiation. */
    if(!result && emos_get_mode()==EMOS_MODE_EXCLUSIVE_COMPAT)
        emos_admission_leave();
    else emos_admission_reset();
    return result;
}
