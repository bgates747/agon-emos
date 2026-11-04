/* INTEG-010: idle-CLI ExCom adapter, not application-state migration.
 * UART1 assembly stays private. All reply effects run in the existing ISR,
 * or under an interrupt lock when the coordinator publishes staged state.
 * Ordinary version1 console controls retain their CRC; parallel format4 has
 * none (PORT-008 CRC-REMOVAL-R01). Neither uses General Poll for activation.
 */
#include <string.h>
#include "emos_console.h"
#include "emos_console_wire.h"
#include "emos_parallel_wire.h"
#include "emos_parallel_handover.h"
#include "emos_keyboard.h"
#include "uart.h"
#if defined(EMOS_EXEXT_STREAM)
#include "emos_exext.h"
#define PARALLEL_CONSOLE_BUSY (emos_console_owned && !emos_exext_owned)
#define EDP_PACKET_ROUTE (emosVduBackend == 1 || (emosVduBackend == 2 && emos_exext_owned))
#else
#define PARALLEL_CONSOLE_BUSY emos_console_owned
#define EDP_PACKET_ROUTE (emosVduBackend == 1)
#endif

extern volatile BYTE emosVduBackend;
extern BYTE vdp_protocol_data[];
volatile BYTE emos_console_owned;
/* INTEG-011: one explicit CLI/API request, cleared by its caller on return. */
BYTE emos_console_keep;
static BYTE request[CONSOLE_SIZE], transaction[4];
static volatile BYTE waiting, accepted, staging, seen, poll_done;
static BYTE staged_mode[8], staged_cursor[2], poll_token;
static BYTE lease;
#if defined(EMOS_EXEXT_STREAM)
/* Parallel transactions borrow request, never the live version1 lease. */
static BYTE lease_nonce[4];
static void restore_console_request(void) {
    memset(request,0,sizeof(request));
    request[0]='E';request[1]='X';request[2]=request[12]=1;
    memcpy(request+4,transaction,4);memcpy(request+8,lease_nonce,4);
}
#endif

static BYTE wait_reply(volatile BYTE *flag) {
    BYTE last = emos_keyboard_clock();
    UINT16 ticks = 0;
    UINT32 budget = 262144UL;
    while (!*flag && !emos_key_faulted && ticks < 600 && budget) {
        BYTE now = emos_keyboard_clock(), delta = (BYTE)(now - last);
        ticks += delta; last = now;
        if (delta) budget = 262144UL; else --budget;
    }
    return *flag && !emos_key_faulted;
}
/* Parallel controls use the reserved serializer; never bypass its owner. */
static BYTE control_send(const BYTE *p, UINT16 n) {
    return request[2] == PARALLEL_WIRE_VERSION ?
        emos_keyboard_parallel_send(p,n) : emos_keyboard_send(p,n);
}
static BYTE exchange(BYTE op) {
    BYTE prefix[3] = {23, 0, CONSOLE_OPCODE};
    BYTE irq, ok;
    request[3] = op;
    if (request[2] != PARALLEL_WIRE_VERSION) request[13] = 0;
    if (request[2] == PARALLEL_WIRE_VERSION) parallel_control_pad(request);
    else console_seal(request);
    irq = emos_keyboard_lock(); accepted = 0; waiting = op;
    emos_keyboard_unlock(irq);
    ok = control_send(prefix, 3) == EMOS_KEY_OK &&
         control_send(request, sizeof(request)) == EMOS_KEY_OK && wait_reply(&accepted);
    irq = emos_keyboard_lock(); waiting = 0; emos_keyboard_unlock(irq);
    return ok;
}
static void effect(BYTE command, BYTE *p, BYTE length) {
    BYTE saved[16];
    /* IRQs are masked: borrow the stock EFFECT buffer, not its parser state.
     * Restore every byte, including a partly assembled UART0 packet. This
     * retains upstream handlers rather than duplicating sysvar semantics.
     * No KEY packet is dispatched by this bridge. The private diagnostic
     * callback observes the borrowed buffer only during this ISR call. */
    memcpy(saved, vdp_protocol_data, 16);
    memcpy(vdp_protocol_data, p, length);
    emos_vdp_effect(command);
    memcpy(vdp_protocol_data, saved, 16);
}
void emos_console_packet(BYTE command, BYTE *p, BYTE length) {
    static const BYTE lengths[10] = {1,4,2,1,4,2,8,6,5,10};
    if (command == CONSOLE_REPLY) {
        if (length != CONSOLE_SIZE || !waiting || accepted ||
            p[2] != request[2] ||
            p[3] != (BYTE)(waiting | 0x80)) return;
#if defined(EMOS_EXEXT_STREAM)
        if (request[2] == PARALLEL_WIRE_VERSION &&
            (waiting == PARALLEL_STREAM_START || waiting == PARALLEL_STREAM_POLL ||
             waiting == PARALLEL_STREAM_STOP)) {
            if (memcmp(p,request,3) || memcmp(p+4,request+4,6) || p[12] || p[13] ||
                p[11]>16 || (p[11]==16 && p[10])) return;
        } else
#endif
        if (request[2] == PARALLEL_WIRE_VERSION && waiting == PARALLEL_OFFER) {
            /* Length and direction are admission identity. Completion uses
             * the physical return handshake, without a later wire receipt. */
            if (!parallel_block_matches(request,p,(BYTE)(waiting|0x80))) return;
        } else {
            if (!(request[2] == PARALLEL_WIRE_VERSION ? parallel_session_valid(p) : console_valid(p)) ||
                p[13] || memcmp(p+4,request+4,4)) return;
            if (waiting == CONSOLE_PREPARE || waiting == CONSOLE_PREPARE_KEEP) {
                if (!(p[8] | p[9] | p[10] | p[11])) return;
                memcpy(request+8,p+8,4);
            } else if (memcmp(p+8,request+8,4)) return;
        }
        /* Foreground owns session transitions. Capture exactly one complete
         * parallel reply; late duplicates cannot replace its nonce. */
        if (request[2] == PARALLEL_WIRE_VERSION) memcpy(request,p,CONSOLE_SIZE);
        accepted = 1;
        return;
    }
    /* INTEG-012 diagnostic packets have their own type. Never relax KEY's
     * four-byte validation or publish these sixteen bytes as keyboard state. */
    if (command == 0x8C) {
        if (length == 16 && EDP_PACKET_ROUTE && lease) effect(command,p,length);
        return;
    }
    if (command < 0x80 || command > 0x89 || command == 0x81 || command == 0x88 ||
        length != lengths[command - 0x80]) return;
    if (staging) {
        if (command == 0x86) { memcpy(staged_mode,p,8); seen |= 1; }
        if (command == 0x82) { memcpy(staged_cursor,p,2); seen |= 2; }
        if (command == 0x80 && p[0] == poll_token && seen == 3) poll_done = 1;
    } else if (EDP_PACKET_ROUTE && lease) effect(command,p,length);
}
BYTE emos_console_prepare(void) {
    BYTE i;
    if (lease || emos_console_owned) return 0;
    if (emos_keyboard_transport_claim() != EMOS_KEY_OK) return 0;
    emos_console_owned = 1;
    for (i=0; i<4; ++i) if (++transaction[i]) break;
    memset(request,0,sizeof(request));
    request[0]='E'; request[1]='X'; request[2]=1; request[12]=1;
    memcpy(request+4,transaction,4);
    if (!exchange(emos_console_keep ? CONSOLE_PREPARE_KEEP : CONSOLE_PREPARE)) return 0;
#if defined(EMOS_EXEXT_STREAM)
    memcpy(lease_nonce,request+8,4);
#endif
    lease = 1;
    return 1;
}
BYTE emos_console_commit(void) {
    BYTE irq, ok;
    BYTE init[] = {26,12,23,1,1,23,0,0x86,23,0,0x82,23,0,0x80,0};
    if (!lease || !exchange(CONSOLE_COMMIT)) return 0;
    irq = emos_keyboard_lock(); staging = 1; seen = poll_done = 0;
    poll_token = request[8] ^ request[4]; init[14] = poll_token;
    emos_keyboard_unlock(irq);
    /* Keep requests still query/synchronize; only viewport/clear/cursor reset
     * is omitted. Never publish stale display sysvars on route commit. */
    if (emos_console_keep) ok = emos_keyboard_send(init+5,10);
    else ok = emos_keyboard_send(init,sizeof(init));
    ok = ok == EMOS_KEY_OK && wait_reply(&poll_done);
    return ok;
}
void emos_console_publish(void) {
    BYTE irq = emos_keyboard_lock();
    if (staging && seen == 3 && poll_done) {
        effect(0x86,staged_mode,8); effect(0x82,staged_cursor,2);
    }
    staging = 0;
    emos_keyboard_unlock(irq);
}
BYTE emos_console_recover(void) {
    BYTE ok = 1;
#if defined(EMOS_EXEXT_STREAM)
    if (!emos_exext_stop()) return 0;
    if (lease) restore_console_request();
#endif
    if (lease) ok = exchange(emosVduBackend ? CONSOLE_LEAVE : CONSOLE_ABORT);
    /* A failed entry never published EDP. Abandon its local lease even if
     * the peer disappeared. An active route must not falsely report LEAVE. */
    if (!ok && emosVduBackend) return 0;
    lease = staging = waiting = emos_console_owned = 0;
    emos_keyboard_transport_release();
    return 1;
}
BYTE emos_console_write_stream(const BYTE *p, UINT16 length) {
    if (!lease || emosVduBackend != 1 || staging) return EMOS_KEY_BUSY;
    return emos_keyboard_send(p,length);
}
BYTE emos_console_write_byte(BYTE value) { return emos_console_write_stream(&value,1); }
void emos_console_notice(BYTE exclusive) {
    const char *s = exclusive ? "Exclusive Compatible mode\r\nOutput is on Extender.\r\n" : "Legacy mode\r\n";
    if (!emos_console_keep) {
        emos_mainboard_put(12);
        while (*s) emos_mainboard_put((BYTE)*s++);
        emos_mainboard_put(23); emos_mainboard_put(1); emos_mainboard_put(!exclusive);
    }
    if (!exclusive) {
        emos_mainboard_put(23); emos_mainboard_put(0); emos_mainboard_put(0x86);
        emos_mainboard_put(23); emos_mainboard_put(0); emos_mainboard_put(0x82);
    }
}

/* Parallel admission is CRC-free (PORT-008 CRC-REMOVAL-R01). Retain the
 * semantic/ownership gates and actual handover; former CRC slots are ignored.
 * This is a coordinator-reserved foreground call, not a UART ISR GPIO entry.
 */
BYTE emos_parallel_handover_admit(t_emosParallelHandover *h, BYTE mode,
    BYTE *session, const BYTE *offer, const BYTE *ack) {
    if (mode != PARALLEL_EXEXT || h->phase != EPH_UART ||
        !parallel_offer_valid(session, offer) || ack[3] != PARALLEL_ACK ||
        memcmp(offer, ack, 3) || memcmp(offer + 4, ack + 4, 10)) return 0;
    if (!emos_parallel_handover_begin(h)) return 0;
    session[4] = offer[8]; session[5] = offer[9];
    return 1;
}

/* Private session owner shares the console request/reply buffer, not its CRC.
 * The UART reservation is checked, not accepted as caller-supplied evidence.
 */
BYTE emos_parallel_session_start(t_emosParallelHandover *h, t_parallelSession *s,
    const BYTE *transaction, BYTE *request) {
    return h->phase == EPH_UART && emos_keyboard_parallel_reserved() &&
        parallel_session_begin(s, transaction, request);
}
BYTE emos_parallel_session_accept(t_emosParallelHandover *h, t_parallelSession *s,
    const BYTE *reply, BYTE *commit) {
    if (h->phase != EPH_UART || !emos_keyboard_parallel_reserved()) return 0;
    return parallel_session_accept(s, reply, commit);
}
BYTE emos_parallel_session_admit(t_emosParallelHandover *h, BYTE mode,
    t_parallelSession *s, const BYTE *offer, const BYTE *ack) {
    return s->phase == PARALLEL_SESSION_ACTIVE && emos_keyboard_parallel_reserved() &&
        emos_parallel_handover_admit(h, mode, s->bytes, offer, ack);
}
void emos_parallel_session_cancel(t_emosParallelHandover *h, t_parallelSession *s) {
    parallel_session_invalidate(s); emos_parallel_handover_cancel(h);
}

/* Private foreground entry owned by the physical coordinator.
 * Borrow the existing console request buffer only while its UART serializer
 * is reserved. On uncertain delivery retain that reservation and request
 * release recovery; neither a timeout nor this function can restore pins.
 */
BYTE emos_parallel_session_negotiate(t_emosParallelHandover *h,
    t_parallelSession *s, const BYTE *transaction) {
    /* An existing ExCom lease still needs its own request identity for LEAVE.
     * The coordinator must close it before borrowing this buffer. */
    if (PARALLEL_CONSOLE_BUSY || waiting ||
        !emos_parallel_session_start(h,s,transaction,request)) return 0;
    do {
        BYTE op = request[3];
        if (!exchange(op) || !emos_parallel_session_accept(h,s,request,request)) {
            emos_parallel_session_cancel(h,s); return 0;
        }
    } while (s->phase != PARALLEL_SESSION_ACTIVE);
    return 1;
}

/* Private foreground block transaction, not a GPIO entry or application API.
 * The physical coordinator retains the offer through completion and owns
 * drain/park/native/return. An ACK only starts the existing handover machine.
 * Any uncertain wire result invalidates the session without restoring pads.
 */
BYTE emos_parallel_block_offer(t_emosParallelHandover *h, t_parallelSession *s,
    BYTE mode, const BYTE *offer) {
    if (PARALLEL_CONSOLE_BUSY || waiting || mode != PARALLEL_EXEXT ||
        h->phase != EPH_UART || s->phase != PARALLEL_SESSION_ACTIVE ||
        !emos_keyboard_parallel_reserved() || !parallel_offer_valid(s->bytes,offer)) return 0;
    memcpy(request,offer,CONSOLE_SIZE);
    if (exchange(PARALLEL_OFFER) && emos_parallel_session_admit(h,mode,s,offer,request)) return 1;
    emos_parallel_session_cancel(h,s); return 0;
}
#if defined(EMOS_EXEXT_STREAM)
/* Queue/status control carries no VDU or returned packet payload. */
BYTE emos_parallel_stream_control(t_parallelSession *s, BYTE op, UINT16 *pending) {
    if (!emos_exext_owned || waiting || s->phase!=PARALLEL_SESSION_ACTIVE ||
        !emos_keyboard_parallel_reserved()) return 0;
    memset(request,0,sizeof(request));memcpy(request,"EX",2);
    request[2]=PARALLEL_WIRE_VERSION;
    memcpy(request+4,s->bytes,6);
    if (!exchange(op)) return 0;
    *pending=(UINT16)request[10] | ((UINT16)request[11]<<8);
    return 1;
}
#endif

#if defined(EMOS_PARALLEL_PROBE) && EMOS_PARALLEL_PROBE
/* PORT-008 LC03-A: fixed private MOSlet composition only. Gateway validates
 * the complete MOSlet output span, direction and length before this call.
 * Keep negotiation/admission/physical ownership in EMOS; MOSlet has no private
 * symbol addresses or GPIO authority. Descriptor precedes provisional data.
 * An admitted failure retains the normal fault fence/reservation until reset.
 */
BYTE emos_parallel_probe(BYTE *out, UINT16 length, BYTE reverse) {
    static t_parallelSession session;
    static BYTE transaction[4];
    t_emosParallelHandover *h = emos_parallel_boot_handover();
    BYTE i;
    if (emos_console_owned || h->phase!=EPH_UART || !emos_parallel_boot_uart_allowed() ||
        emos_keyboard_parallel_reserve() != UART_POLL_READY) return 0;
    for (i=0; i<4; ++i) if (++transaction[i]) break;
    if (!emos_parallel_session_negotiate(h,&session,transaction)) goto failed;
    memset(out,0,16); memcpy(out,"EX",2);
    out[2]=PARALLEL_WIRE_VERSION;out[3]=PARALLEL_OFFER;
    memcpy(out+4,session.bytes,4); out[8]=1;
    out[10]=(BYTE)length;out[11]=(BYTE)(length>>8);out[12]=reverse;
    parallel_control_pad(out);
    if (emos_parallel_block_offer(h,&session,PARALLEL_EXEXT,out) &&
        emos_parallel_native_coordinate(&session,out,out+16,length)) return 1;
 failed:
    /* Even an uncertain PREPARE/OFFER must enter the real release fence; a
     * logical session cancellation alone does not disconnect UART outputs. */
    emos_parallel_boot_block_end(0);
    return 0;
}
#endif
