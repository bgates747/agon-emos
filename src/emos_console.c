/* INTEG-010: idle-CLI ExCom adapter, not application-state migration.
 * UART1 assembly stays private. All reply effects run in the existing ISR,
 * or under an interrupt lock when the coordinator publishes staged state.
 * Control is a framed CRC-checked exchange, never a General Poll activation.
 */
#include <string.h>
#include "emos_console.h"
#include "emos_console_wire.h"
#include "emos_keyboard.h"
#include "uart.h"

extern volatile BYTE emosVduBackend;
extern BYTE vdp_protocol_data[];
volatile BYTE emos_console_owned;
/* INTEG-011: one explicit CLI/API request, cleared by its caller on return. */
BYTE emos_console_keep;
static BYTE request[CONSOLE_SIZE], transaction[4];
static volatile BYTE waiting, accepted, staging, seen, poll_done;
static BYTE staged_mode[8], staged_cursor[2], poll_token;
static BYTE lease;

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
static BYTE exchange(BYTE op) {
    BYTE prefix[3] = {23, 0, CONSOLE_OPCODE};
    BYTE irq, ok;
    request[3] = op; request[13] = 0; console_seal(request);
    irq = emos_keyboard_lock(); accepted = 0; waiting = op;
    emos_keyboard_unlock(irq);
    ok = emos_keyboard_send(prefix, 3) == EMOS_KEY_OK &&
         emos_keyboard_send(request, sizeof(request)) == EMOS_KEY_OK && wait_reply(&accepted);
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
        if (length != CONSOLE_SIZE || !waiting || !console_valid(p) ||
            p[3] != (BYTE)(waiting | 0x80) || p[13] ||
            memcmp(p + 4, request + 4, 4)) return;
        if (waiting == CONSOLE_PREPARE || waiting == CONSOLE_PREPARE_KEEP) {
            if (!(p[8] | p[9] | p[10] | p[11])) return;
            memcpy(request + 8, p + 8, 4);
        } else if (memcmp(p + 8, request + 8, 4)) return;
        accepted = 1;
        return;
    }
    /* INTEG-012 diagnostic packets have their own type. Never relax KEY's
     * four-byte validation or publish these sixteen bytes as keyboard state. */
    if (command == 0x8C) {
        if (length == 16 && emosVduBackend == 1 && lease) effect(command,p,length);
        return;
    }
    if (command < 0x80 || command > 0x89 || command == 0x81 || command == 0x88 ||
        length != lengths[command - 0x80]) return;
    if (staging) {
        if (command == 0x86) { memcpy(staged_mode,p,8); seen |= 1; }
        if (command == 0x82) { memcpy(staged_cursor,p,2); seen |= 2; }
        if (command == 0x80 && p[0] == poll_token && seen == 3) poll_done = 1;
    } else if (emosVduBackend == 1 && lease) effect(command,p,length);
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
    if (lease) ok = exchange(emosVduBackend == 1 ? CONSOLE_LEAVE : CONSOLE_ABORT);
    /* A failed entry never published EDP. Abandon its local lease even if
     * the peer disappeared. An active route must not falsely report LEAVE. */
    if (!ok && emosVduBackend == 1) return 0;
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
