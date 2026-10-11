/* PORT-008 EX02: ordinary, EMOS-owned ExExt stream on the qualified block
 * coordinator. Replaces the obsolete indefinite forward-only epoch binding
 * ONLY in the explicit experimental profile. Apps keep RST 10/18 unchanged.
 * UART carries versioned control only; VDU and returned packets use parallel.
 * Byte writes batch until full or VBlank. One owner excludes nested writers.
 * The real VBlank bridge temporarily admits UART interrupts for bounded control
 * exchanges, then masks interrupts before feeding the existing packet parser.
 * No callback is invoked in invented foreground interrupt context.
 */
#include <string.h>
#include "emos_exext.h"
#include "emos_parallel.h"
#include "emos_parallel_handover.h"
#include "emos_console.h"
#include "emos_console_wire.h"
#include "emos_keyboard.h"
#include "uart.h"
extern volatile BYTE emosVduBackend;
extern void emos_keyboard_byte(BYTE);
extern BYTE emos_parallel_stream_control(t_parallelSession *, BYTE, UINT16 *);
static t_parallelSession stream_session;
static BYTE stream_transaction[4], offer[16];
static BYTE outgoing[4096], incoming[256];
static UINT16 queued;
static volatile UINT16 received;
static volatile BYTE portc_reserved;
volatile BYTE emos_exext_owned, emos_exext_busy;

/* Native boot owns initial pad release. This profile has no indefinite/raw
 * epoch entry; UART open and the block coordinator share this atomic guard. */
BYTE emos_parallel_uart1_guard_acquire(void) {
    BYTE irq = emos_keyboard_lock(), ok = !portc_reserved;
    if (ok) portc_reserved = 1;
    emos_keyboard_unlock(irq);
    return ok ? EMOS_PARALLEL_OK : EMOS_PARALLEL_BUSY;
}
void emos_parallel_uart1_guard_release(void) { portc_reserved = 0; }

static BYTE transfer(BYTE *p, UINT16 length, BYTE reverse) {
    UINT16 next = (UINT16)(stream_session.bytes[4] |
        ((UINT16)stream_session.bytes[5] << 8)) + 1;
    t_emosParallelHandover *h = emos_parallel_boot_handover();
    if (emos_keyboard_parallel_reserve() != UART_POLL_READY) return 0;
    /* Renew only at the proved UART boundary, before a block number wraps.
     * No outstanding payload is replayed; the ordinary console lease and
     * stream queues survive this fresh private transport capability. */
    if(!next) {
        BYTE i;
        for(i=0;i<4;++i)if(++stream_transaction[i])break;
        if(!emos_parallel_session_negotiate(h,&stream_session,stream_transaction)) {
            emos_parallel_boot_block_end(0);return 0;
        }
        next=1;
    }
    memset(offer,0,sizeof(offer)); memcpy(offer,"EX\x02\x02",4);
    memcpy(offer+4,stream_session.bytes,4);
    offer[8]=(BYTE)next; offer[9]=(BYTE)(next>>8);
    offer[10]=(BYTE)length; offer[11]=(BYTE)(length>>8); offer[12]=reverse;
    console_seal(offer);
    if (emos_parallel_block_offer(h,&stream_session,PARALLEL_EXEXT,offer) &&
        emos_parallel_native_coordinate(&stream_session,offer,p,length)) return 1;
    emos_parallel_boot_block_end(0); return 0;
}
static BYTE flush(void) {
    if (!queued) return 1;
    if (!transfer(outgoing,queued,0)) return 0;
    queued=0; return 1;
}
static BYTE poll_return(void) {
    UINT16 pending=0;
    BYTE ok;
    if (emos_keyboard_parallel_reserve()!=UART_POLL_READY) return 0;
    ok=emos_parallel_stream_control(&stream_session,PARALLEL_STREAM_POLL,&pending);
    if (emos_keyboard_parallel_release()!=UART_POLL_READY) ok=0;
    if (!ok) return 0;
    if (pending>sizeof(incoming)) pending=sizeof(incoming);
    if (pending && !transfer(incoming,pending,1)) return 0;
    received=pending;
    return 1;
}
BYTE emos_exext_start(void) {
    BYTE i, ok; UINT16 unused;
    t_emosParallelHandover *h=emos_parallel_boot_handover();
    if (emos_exext_owned || emos_exext_busy || !emos_console_owned ||
        h->phase!=EPH_UART || !emos_parallel_boot_uart_allowed()) return 0;
    emos_exext_busy=emos_exext_owned=1; queued=received=0;
    for (i=0;i<4;++i) if (++stream_transaction[i]) break;
    if (emos_keyboard_parallel_reserve()!=UART_POLL_READY) goto failed;
    ok=emos_parallel_session_negotiate(h,&stream_session,stream_transaction) &&
        emos_parallel_stream_control(&stream_session,PARALLEL_STREAM_START,&unused);
    if (emos_keyboard_parallel_release()!=UART_POLL_READY) ok=0;
    if (!ok) goto failed;
    emos_exext_busy=0; return 1;
 failed:
    emos_exext_owned=emos_exext_busy=0; return 0;
}
BYTE emos_exext_stop(void) {
    BYTE ok, irq; UINT16 pending=0;
    t_emosDeadline d;
    if (!emos_exext_owned) return 1;
    if (emos_exext_busy) return 0;
    emos_exext_busy=1;
    ok=flush();
    d.last=emos_keyboard_clock(); d.elapsed=0; d.budget=(UINT24)262144;
    /* STOP acknowledges queued return bytes without closing the stream. Drain
     * those bytes and let a real VBlank publish them before retrying STOP.
     * A key arriving between POLL and STOP must not strand the transition. */
    do {
        while(received && emos_keyboard_deadline_step(&d)) { }
        if(received || !ok || emos_key_faulted ||
            emos_keyboard_parallel_reserve()!=UART_POLL_READY) { ok=0; break; }
        ok=emos_parallel_stream_control(&stream_session,PARALLEL_STREAM_STOP,&pending);
        if(emos_keyboard_parallel_release()!=UART_POLL_READY)ok=0;
        if(!ok || !pending)break;
        if(pending>sizeof(incoming))pending=sizeof(incoming);
        ok=transfer(incoming,pending,1);
        if(ok) {
            irq=emos_keyboard_lock();received=pending;emos_keyboard_unlock(irq);
        }
    } while(emos_keyboard_deadline_step(&d));
    if(pending || received)ok=0;
    if (ok) { emos_exext_owned=0; queued=received=0; }
    emos_exext_busy=0; return ok;
}
BYTE emos_parallel_route_write_stream(const BYTE *p, UINT16 n) {
    BYTE irq=emos_keyboard_lock(), ok=1;
    if (!emos_exext_owned || emos_exext_busy || emosVduBackend!=2 || !irq) {
        emos_keyboard_unlock(irq); return EMOS_PARALLEL_BUSY;
    }
    emos_exext_busy=1; emos_keyboard_unlock(irq);
    while(n && ok) {
        UINT16 part=sizeof(outgoing)-queued;
        if(part>n)part=n;
        memcpy(outgoing+queued,p,part);queued+=part;p+=part;n-=part;
        if(queued==sizeof(outgoing))ok=flush();
    }
    emos_exext_busy=0;
    return ok ? EMOS_PARALLEL_OK : EMOS_PARALLEL_COMPLETION_TIMEOUT;
}
BYTE emos_parallel_route_write_byte(BYTE value) {
    return emos_parallel_route_write_stream(&value,1);
}
void emos_exext_tick(void) {
    UINT16 i;
    /* A foreground mode transition may own transport while waiting for the
     * preceding return block's actual ISR effects. Publication itself never
     * negotiates a block and therefore precedes the non-reentrancy guard. */
    for(i=0;i<received && !emos_key_faulted;++i) emos_keyboard_byte(incoming[i]);
    received=0;
    if (!emos_exext_owned || emos_exext_busy || emosVduBackend!=2 || emos_key_faulted) return;
    /* Caller is the fully saved real VBlank handler, with interrupts masked.
     * The assembly bridge holds busy while admitting UART interrupts, then
     * returns masked. Nested ticks/foreground output cannot take this owner. */
    emos_exext_busy=1;
    emos_exext_interrupt_service();
    for(i=0;i<received && !emos_key_faulted;++i) emos_keyboard_byte(incoming[i]);
    received=0; emos_exext_busy=0;
}
void emos_exext_service(void) {
    /* Only the interrupt bridge calls this, with busy already held. */
    if (!flush() || !poll_return()) {
        emos_parallel_boot_block_end(0);
        emos_exext_owned=0;
    }
}
