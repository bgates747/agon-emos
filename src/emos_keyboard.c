/* INTEG-009: resident keyboard source and private stock UART1 framing.
 * No module loader, foreground input pump, or mainboard VDU redirection.
 * All packet effects and synthetic releases run under a real interrupt.
 * Stock silent-idle UART has no heartbeat: a dead P4 cannot always be detected.
 */
#include <string.h>
#include "emos_keyboard.h"
#include "uart.h"

volatile BYTE emos_key_source, emos_key_faulted;
static volatile BYTE transitioning, preparing, stop_requested, fault_requested;
static BYTE layout, token;
static volatile BYTE text_pending;
static BYTE state, command, length, remaining, used, payload[5], partial_at;
static BYTE held[32], modifiers, zero_down;

static void parser_reset(void) { state = remaining = used = 0; }
static BYTE valid_key(const BYTE *p) {
    return p[2] <= EMOS_KEY_MAX && p[3] <= 1;
}
static void publish(BYTE *p) {
    /* The assembly bridge invokes the stock callback BEFORE publication;
     * callback edits to the same payload remain authoritative. */
    emos_keyboard_effect(p);
    modifiers = p[1];
    if (!p[2]) zero_down = p[3] & 1;
    if (p[2] && p[2] <= EMOS_KEY_MAX) {
        BYTE mask = (BYTE)(1U << (p[2] & 7));
        if (p[3] & 1) held[p[2] >> 3] |= mask;
        else held[p[2] >> 3] &= (BYTE)~mask;
    }
}
static void release_key(BYTE key) {
    BYTE p[4];
    if (!(held[key >> 3] & (1U << (key & 7)))) return;
    p[0] = 0; p[1] = modifiers & 0x70; p[2] = key; p[3] = 0;
    publish(p);
}
static void cleanup(void) {
    UINT key;
    /* Stock virtual modifiers 117..124 first. Clear physical modifier bits
     * in every synthetic payload; retain lock bits separately. In particular
     * never synthesize Ctrl+Alt+Delete key-up, which stock MOS uses to reset.
     * A registered callback can deliberately edit releases as in stock MOS;
     * bounded cleanup never loops waiting for that callback to cooperate. */
    for (key = 117; key <= 124; ++key) release_key((BYTE)key);
    for (key = 1; key <= EMOS_KEY_MAX; ++key)
        if (key < 117 || key > 124) release_key((BYTE)key);
    if (zero_down || (modifiers & 0x8F)) {
        BYTE p[4] = {0, 0, 0, 0};
        p[1] = modifiers & 0x70;
        publish(p); /* Also clear modifiers supplied with virtual key zero. */
    }
    memset(held, 0, sizeof(held));
    modifiers &= 0x70;
    zero_down = 0;
}
void emos_keyboard_mainboard(BYTE *p) {
    if (emos_key_source == EMOS_KEY_MAINBOARD && valid_key(p)) publish(p);
}
void emos_keyboard_mainboard_settings(BYTE *p) {
    if (emos_key_source == EMOS_KEY_MAINBOARD) emos_keyboard_settings(p);
}
void emos_keyboard_fault(void) {
    /* ISR only. Foreground TX errors request this through fault_requested. */
    uart1_keyboard_stop();
    parser_reset();
    preparing = text_pending = 0;
    emos_key_faulted = 1;
    if (emos_key_source == EMOS_KEY_BROWSER) cleanup();
}
void emos_keyboard_tick(void) {
    if (stop_requested) {
        uart1_keyboard_stop();
        cleanup();
        parser_reset();
        preparing = 0;
        emos_key_source = EMOS_KEY_MAINBOARD;
        emos_key_faulted = fault_requested = stop_requested = 0;
    } else if (uart1_keyboard_owned && !emos_key_faulted &&
               (fault_requested || (state &&
                 (BYTE)(emos_keyboard_clock() - partial_at) >= 30))) {
        /* Mainboard VBlank adds two clock units per 60 Hz tick: 30 = 250ms.
         * Whole-frame age, not interbyte progress; a dribble cannot extend it. */
        fault_requested = 0;
        emos_keyboard_fault();
    }
}
static void dispatch(void) {
    if (command == 0x80 && length == 1 && preparing && payload[0] == token) {
        cleanup();
        emos_key_source = EMOS_KEY_BROWSER;
        preparing = 0;
    } else if (!preparing && emos_key_source == EMOS_KEY_BROWSER) {
        if (command == 0x81) {
            if (!valid_key(payload)) emos_keyboard_fault();
            else publish(payload);
        } else if (command == 0x88) emos_keyboard_settings(payload);
        else if (command == 0x80 && length == 1 && text_pending && payload[0] == token)
            text_pending = 0;
    }
}
void emos_keyboard_byte(BYTE value) {
    if (emos_key_faulted || !uart1_keyboard_owned) return;
    if (!state) {
        if (!(value & 0x80)) return;
        command = value; state = 1; partial_at = emos_keyboard_clock();
    } else if (state == 1) {
        length = remaining = value; used = 0;
        if ((command == 0x81 && value != 4) || (command == 0x88 && value != 5)) {
            emos_keyboard_fault(); return;
        }
        /* Consume unowned/oversize bodies by length. Never rescan payload
         * bytes as headers, including when their high bit is set. */
        state = 2;
        if (!remaining) parser_reset();
    } else {
        if (used < sizeof(payload)) payload[used++] = value;
        if (!--remaining) {
            state = 0;
            if (length <= sizeof(payload)) dispatch();
        }
    }
}

typedef struct { BYTE last; UINT16 elapsed; UINT32 budget; } Deadline;
static void deadline_start(Deadline *d) {
    d->last = emos_keyboard_clock(); d->elapsed = 0; d->budget = 262144UL;
}
static BYTE deadline_step(Deadline *d) {
    BYTE now = emos_keyboard_clock(), delta = (BYTE)(now - d->last);
    d->elapsed += delta; d->last = now;
    if (delta) d->budget = 262144UL;
    else if (!--d->budget) return 0;
    return d->elapsed < 600;
}
static BYTE transmit(BYTE value, Deadline *d) {
    BYTE result;
    do {
        if (emos_key_faulted || !deadline_step(d)) return 0;
        result = uart1_keyboard_put(value);
        if (result == UART_POLL_ERROR) { fault_requested = 1; return 0; }
        if (result == UART_POLL_UNAVAILABLE) return 0;
    } while (result != UART_POLL_READY);
    return 1;
}
static BYTE setting(BYTE value, Deadline *d) {
    return transmit(23, d) && transmit(0, d) && transmit(0x81, d) && transmit(value, d);
}
BYTE emos_keyboard_select(BYTE source) {
    BYTE irq, result = EMOS_KEY_OK, previous_fault;
    Deadline d;
    irq = emos_keyboard_lock();
    if (transitioning || !irq) { emos_keyboard_unlock(irq); return EMOS_KEY_BUSY; }
    if (source == emos_key_source && !emos_key_faulted) {
        emos_keyboard_unlock(irq); return EMOS_KEY_OK;
    }
    transitioning = 1;
    previous_fault = emos_key_faulted;
    emos_keyboard_unlock(irq);
    deadline_start(&d);
    if (source == EMOS_KEY_MAINBOARD) {
        emos_keyboard_mainboard_layout(layout);
        stop_requested = 1;
        while (stop_requested && deadline_step(&d)) { }
        irq = emos_keyboard_lock();
        if (stop_requested) { stop_requested = 0; result = EMOS_KEY_TIMEOUT; }
        else uart1_keyboard_close();
        emos_keyboard_unlock(irq);
    } else {
        irq = emos_keyboard_lock();
        if (uart1_keyboard_owned) uart1_keyboard_close(); /* explicit fault retry */
        parser_reset();
        emos_key_faulted = fault_requested = 0;
        preparing = 1;
        ++token;
        if (uart1_keyboard_open() != UART_POLL_READY) result = EMOS_KEY_BUSY;
        emos_keyboard_unlock(irq);
        if (result == EMOS_KEY_OK &&
            (!setting(layout, &d) || !transmit(23, &d) || !transmit(0, &d) ||
             !transmit(0x80, &d) || !transmit(token, &d))) result = EMOS_KEY_TIMEOUT;
        while (result == EMOS_KEY_OK && preparing && !emos_key_faulted && deadline_step(&d)) { }
        irq = emos_keyboard_lock();
        if (result == EMOS_KEY_OK && (preparing || emos_key_faulted)) result = EMOS_KEY_TIMEOUT;
        if (result != EMOS_KEY_OK) {
            preparing = 0; parser_reset(); uart1_keyboard_close();
            emos_key_faulted = previous_fault;
        }
        emos_keyboard_unlock(irq);
    }
    transitioning = 0;
    return result;
}
BYTE emos_keyboard_layout(BYTE value) {
    BYTE irq = emos_keyboard_lock(), ok;
    Deadline d;
    if (transitioning || !irq) { emos_keyboard_unlock(irq); return EMOS_KEY_BUSY; }
    transitioning = 1;
    emos_keyboard_unlock(irq);
    if (emos_key_source == EMOS_KEY_MAINBOARD) {
        emos_keyboard_mainboard_layout(value); ok = 1;
    } else {
        deadline_start(&d);
        ok = setting(value, &d);
        /* A partial no-reply setting cannot be rolled back. Stop this owned
         * stream rather than appending another command to its truncated body. */
        if (!ok) fault_requested = 1;
    }
    if (ok) layout = value;
    transitioning = 0;
    return ok ? EMOS_KEY_OK : EMOS_KEY_TIMEOUT;
}

/* One foreground writer, one IRQ receiver. Keep keyboard packets flowing while
 * the retained P4 renderer consumes text. A failed partial transaction poisons
 * this stream; only the IRQ may release held keys and latch the fault. */
BYTE emos_keyboard_text(const BYTE *text, UINT16 length) {
    BYTE irq = emos_keyboard_lock(), ok = 1;
    UINT16 n;
    Deadline d;
    if (transitioning || !irq || preparing || emos_key_faulted ||
        emos_key_source != EMOS_KEY_BROWSER || !uart1_keyboard_owned) {
        emos_keyboard_unlock(irq); return EMOS_KEY_BUSY;
    }
    transitioning = 1; text_pending = 1; ++token;
    emos_keyboard_unlock(irq);
    deadline_start(&d);
    for (n = 0; n < length && ok; ++n) ok = transmit(text[n], &d);
    /* Retained drawing flush, followed by an ordered stock General Poll. */
    if (ok) ok = transmit(23,&d) && transmit(0,&d) && transmit(0xCA,&d) &&
                 transmit(23,&d) && transmit(0,&d) && transmit(0x80,&d) && transmit(token,&d);
    while (ok && text_pending && !emos_key_faulted && deadline_step(&d)) { }
    irq = emos_keyboard_lock();
    if (!ok || text_pending || emos_key_faulted) { ok = 0; fault_requested = 1; }
    text_pending = transitioning = 0;
    emos_keyboard_unlock(irq);
    return ok ? EMOS_KEY_OK : EMOS_KEY_TIMEOUT;
}
