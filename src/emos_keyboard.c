/* INTEG-009: resident keyboard source and private stock UART1 framing.
 * No module loader, foreground input pump, or mainboard VDU redirection.
 * All packet effects and synthetic releases run under a real interrupt.
 * Stock silent-idle UART has no heartbeat: a dead P4 cannot always be detected.
 */
#include <string.h>
#include "emos_keyboard.h"
#include "uart.h"
#include "emos_console.h"
#include "emos_sdlink.h"
#include "emos_telemetry.h"

volatile BYTE emos_key_source, emos_key_faulted;
static volatile BYTE transitioning, preparing, stop_requested, fault_requested;
static BYTE prepared_source;
#ifdef EMOS_BENCH_TELEMETRY
/* Core-owned packet: never retain application memory beyond a gateway call.
 * transitioning protects COMPLETE wire packets from foreground interleaving. */
static BYTE async_data[144], async_at;
static volatile BYTE async_length, async_position;
static void async_abort(void) {
    if (async_length) transitioning = 0;
    async_length = 0; /* queue() initializes the next position before admission. */
    uart1_keyboard_tx_enable(0);
}
#endif
static BYTE layout, token;
static volatile BYTE text_pending;
/* RX01 private C/assembly layout: six bytes then 240 payload bytes.
 * Exported only for the local assembly parser; never a public MOS sysvar. */
typedef struct {
    BYTE state, command, length, remaining, used, partial_at;
    BYTE payload[EMOS_SDLINK_LIMIT];
} KeyboardRx;
KeyboardRx emos_key_rx;
typedef char keyboard_rx_layout_must_match[(sizeof(KeyboardRx)==246 &&
                                           EMOS_SDLINK_LIMIT==240) ? 1 : -1];
static BYTE held[32], modifiers, zero_down;

static void parser_reset(void) { emos_key_rx.state = emos_key_rx.remaining = emos_key_rx.used = 0; }
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
    #ifdef EMOS_BENCH_TELEMETRY
    async_abort();
    emos_telemetry_reset();
    #endif
    emos_sdlink_reset();
    parser_reset();
    preparing = text_pending = 0;
    emos_key_faulted = 1;
    if (emos_key_source != EMOS_KEY_MAINBOARD) cleanup();
}
void emos_keyboard_tick(void) {
    if (stop_requested) {
        #ifdef EMOS_BENCH_TELEMETRY
        async_abort();
        emos_telemetry_reset();
        #endif
        emos_sdlink_reset();
        if (!emos_console_owned) uart1_keyboard_stop();
        cleanup();
        if (!emos_console_owned) parser_reset();
        preparing = 0;
        emos_key_source = EMOS_KEY_MAINBOARD;
        emos_key_faulted = fault_requested = stop_requested = 0;
    } else if (uart1_keyboard_owned && !emos_key_faulted &&
               (fault_requested || (emos_key_rx.state &&
                 (BYTE)(emos_keyboard_clock() - emos_key_rx.partial_at) >= 30))) {
        /* Mainboard VBlank adds two clock units per 60 Hz tick: 30 = 250ms.
         * Whole-frame age, not interbyte progress; a dribble cannot extend it. */
        fault_requested = 0;
        emos_keyboard_fault();
    }
#ifdef EMOS_BENCH_TELEMETRY
    if (async_length && !emos_key_faulted) {
        if ((BYTE)(emos_keyboard_clock()-async_at) >= 30) {
            /* Never append a new packet after a timed-out partial record. */
            emos_keyboard_fault();
        } else {
            /* CTS is GPIO flow control, not a modem interrupt: retry on VBlank.
             * No polling wait here; at most 16 ready bytes per invocation. */
            uart1_keyboard_tx_enable(1);
            emos_keyboard_async_irq();
        }
    }
#endif
}
void emos_keyboard_dispatch(void) {
    if (emos_key_rx.command == 0x8D) { emos_sdlink_packet(emos_key_rx.payload,emos_key_rx.length); return; }
    emos_console_packet(emos_key_rx.command, emos_key_rx.payload, emos_key_rx.length);
    if (emos_key_rx.command == 0x80 && emos_key_rx.length == 1 && preparing && emos_key_rx.payload[0] == token) {
        cleanup();
        emos_key_source = prepared_source;
        preparing = 0;
    } else if (!preparing && emos_key_source != EMOS_KEY_MAINBOARD) {
        if (emos_key_rx.command == 0x81) {
            if (!valid_key(emos_key_rx.payload)) emos_keyboard_fault();
            else publish(emos_key_rx.payload);
        } else if (emos_key_rx.command == 0x88) emos_keyboard_settings(emos_key_rx.payload);
        else if (emos_key_rx.command == 0x80 && emos_key_rx.length == 1 && text_pending && emos_key_rx.payload[0] == token)
            text_pending = 0;
    }
}
#ifdef EMOS_RX_BYTE_C_REFERENCE
void emos_keyboard_byte(BYTE value) {
    if (emos_key_faulted || !uart1_keyboard_owned) return;
    if (!emos_key_rx.state) {
        if (!(value & 0x80)) return;
        emos_key_rx.command = value; emos_key_rx.state = 1; emos_key_rx.partial_at = emos_keyboard_clock();
    } else if (emos_key_rx.state == 1) {
        emos_key_rx.length = emos_key_rx.remaining = value; emos_key_rx.used = 0;
        if ((emos_key_rx.command == 0x81 && value != 4) || (emos_key_rx.command == 0x88 && value != 5)) {
            emos_keyboard_fault(); return;
        }
        /* Consume unowned/oversize bodies by length. Never rescan payload
         * bytes as headers, including when their high bit is set. */
        emos_key_rx.state = 2;
        if (!emos_key_rx.remaining) parser_reset();
    } else {
        if (emos_key_rx.used < sizeof(emos_key_rx.payload)) emos_key_rx.payload[emos_key_rx.used++] = value;
        if (!--emos_key_rx.remaining) {
            emos_key_rx.state = 0;
            if (emos_key_rx.length <= sizeof(emos_key_rx.payload)) emos_keyboard_dispatch();
        }
    }
}
#endif

/* INTEG-014 E05: 262144 fits native 24 bits. Preserve the exact poll
 * limit without pulling 32-bit arithmetic helpers into every byte attempt. */
typedef struct { BYTE last; UINT16 elapsed; UINT24 budget; } Deadline;
static void deadline_start(Deadline *d) {
    d->last = emos_keyboard_clock(); d->elapsed = 0; d->budget = (UINT24)262144;
}
static BYTE deadline_step(Deadline *d) {
    BYTE now = emos_keyboard_clock(), delta = (BYTE)(now - d->last);
    /* Most attempts share a clock tick: avoid rewriting unchanged fields. */
    if (delta) {
        d->elapsed += delta; d->last = now; d->budget = (UINT24)262144;
    }
    else if (!--d->budget) return 0;
    return d->elapsed < 600;
}
/* One C frame per block, matching stock's counted-output shape. Keep the
 * deadline and atomic UART ownership/error check on EVERY attempt. */
#ifdef EMOS_TX_BLOCK_C_REFERENCE
static BYTE transmit_block(const BYTE *data, UINT16 length, Deadline *d) {
    BYTE result, value;
    while (length) {
        /* Match the byte helper: retain this byte across blocked retries,
         * even if an interrupt changes the caller's source buffer. */
        value = *data++;
        do {
            if (emos_key_faulted || !deadline_step(d)) return 0;
            result = uart1_keyboard_put(value);
            if (result == UART_POLL_ERROR) { fault_requested = 1; return 0; }
            if (result == UART_POLL_UNAVAILABLE) return 0;
        } while (result != UART_POLL_READY);
        --length;
    }
    return 1;
}
#else
/* Private enabled-IRQ boundary; all four callers enforce this before sending.
 * Deadline is fresh/successful (elapsed <600), private to this foreground call.
 * Target assembly returns 2 only for an acknowledging UART error. */
extern BYTE emos_keyboard_transmit_block(const BYTE *, UINT16, Deadline *);
static BYTE transmit_block(const BYTE *data, UINT16 length, Deadline *d) {
    BYTE result = emos_keyboard_transmit_block(data, length, d);
    if (result == 2) fault_requested = 1;
    return result == 1;
}
#endif
static BYTE transmit(BYTE value, Deadline *d) {
    return transmit_block(&value, 1, d);
}
static BYTE setting(BYTE value, Deadline *d) {
    return transmit(23, d) && transmit(0, d) && transmit(0x81, d) && transmit(value, d);
}
BYTE emos_keyboard_select(BYTE source) {
    BYTE irq, result = EMOS_KEY_OK, previous_fault;
    Deadline d;
    irq = emos_keyboard_lock();
    if (transitioning || !irq) { emos_keyboard_unlock(irq); return EMOS_KEY_BUSY; }
    /* One P4 acquisition composition at a time. General Poll does not select
     * a physical input provider. Until P4 provider switching is implemented,
     * require mainboard between remote selectors; never tear down a healthy
     * receiver and then pretend a failed replacement preserved it. */
    if (source > EMOS_KEY_EXTENDER ||
        (source != EMOS_KEY_MAINBOARD && emos_key_source != EMOS_KEY_MAINBOARD &&
         source != emos_key_source)) {
        emos_keyboard_unlock(irq); return EMOS_KEY_BUSY;
    }
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
        else if (!emos_console_owned) uart1_keyboard_close();
        emos_keyboard_unlock(irq);
    } else {
        irq = emos_keyboard_lock();
        if (uart1_keyboard_owned && (!emos_console_owned || emos_key_faulted))
            uart1_keyboard_close(); /* explicit fault retry */
        if (!emos_console_owned || emos_key_faulted) parser_reset();
        emos_key_faulted = fault_requested = 0;
        preparing = 1;
        prepared_source = source;
        ++token;
        if (!uart1_keyboard_owned && uart1_keyboard_open() != UART_POLL_READY) result = EMOS_KEY_BUSY;
        emos_keyboard_unlock(irq);
        if (result == EMOS_KEY_OK &&
            (!setting(layout, &d) || !transmit(23, &d) || !transmit(0, &d) ||
             !transmit(0x80, &d) || !transmit(token, &d))) result = EMOS_KEY_TIMEOUT;
        while (result == EMOS_KEY_OK && preparing && !emos_key_faulted && deadline_step(&d)) { }
        irq = emos_keyboard_lock();
        if (result == EMOS_KEY_OK && (preparing || emos_key_faulted)) result = EMOS_KEY_TIMEOUT;
        if (result != EMOS_KEY_OK) {
            preparing = 0;
            if (!emos_console_owned) { parser_reset(); uart1_keyboard_close(); }
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
        if (ok && emos_key_source == EMOS_KEY_EXTENDER) {
            /* The native USB candidate supports UK/US. An ordered poll after
             * locale also bounds unsupported/no-progress peers without adding
             * a non-stock packet. It is not firmware/provider authentication. */
            text_pending = 1; ++token;
            ok = transmit(23,&d) && transmit(0,&d) && transmit(0x80,&d) && transmit(token,&d);
            while (ok && text_pending && !emos_key_faulted && deadline_step(&d)) { }
            if (text_pending || emos_key_faulted) ok = 0;
            text_pending = 0;
        }
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

/* Shared resident UART owner. Console lease is separate from keyboard source;
 * no application is allowed to call this private transport boundary. */
BYTE emos_keyboard_transport_claim(void) {
    BYTE irq = emos_keyboard_lock(), result = EMOS_KEY_OK;
    if (!irq || transitioning || preparing || emos_key_faulted) result = EMOS_KEY_BUSY;
    else if (!uart1_keyboard_owned) {
        parser_reset();
        if (uart1_keyboard_open() != UART_POLL_READY) result = EMOS_KEY_BUSY;
    }
    emos_keyboard_unlock(irq);
    return result;
}
void emos_keyboard_transport_release(void) {
    BYTE irq = emos_keyboard_lock();
    if (!emos_console_owned && emos_key_source == EMOS_KEY_MAINBOARD) {
        uart1_keyboard_close(); parser_reset();
    }
    emos_keyboard_unlock(irq);
}
BYTE emos_keyboard_send(const BYTE *data, UINT16 length) {
    BYTE irq = emos_keyboard_lock(), ok = 1;
    Deadline d;
    if (!irq || transitioning || preparing || emos_key_faulted || !uart1_keyboard_owned) {
        emos_keyboard_unlock(irq); return EMOS_KEY_BUSY;
    }
    transitioning = 1;
    emos_keyboard_unlock(irq);
    deadline_start(&d);
    ok = transmit_block(data, length, &d);
    if (!ok) fault_requested = 1; /* Partial packets cannot be safely replayed. */
    transitioning = 0;
    return ok ? EMOS_KEY_OK : EMOS_KEY_TIMEOUT;
}


#ifdef EMOS_BENCH_TELEMETRY
/* BENCH-001 resident asynchronous transmitter. Runs with all CPU registers
 * saved and interrupts disabled from the existing UART/VBlank vector owners.
 * One bounded drain, no waits, filesystem or application callbacks. */
void emos_keyboard_async_irq(void) {
    BYTE budget=16, result;
    /* TX demand is disabled by completion/abort, so ordinary RX interrupts
     * need no UART register write when there is no resident packet. */
    if (!async_length) return;
    while (budget-- && async_position < async_length) {
        result=uart1_keyboard_put(async_data[async_position]);
        if (result == UART_POLL_BLOCKED) {
            uart1_keyboard_tx_enable(0); /* Avoid level-triggered IRQ storm. */
            return;
        }
        if (result == UART_POLL_EMPTY) return;
        if (result != UART_POLL_READY) { emos_keyboard_fault(); return; }
        ++async_position;
    }
    if (async_position == async_length) {
        async_length=0;
        transitioning=0;
        uart1_keyboard_tx_enable(0);
    }
}
BYTE emos_keyboard_queue_telemetry(const BYTE *data) {
    BYTE irq=emos_keyboard_lock();
    if (!irq || transitioning ||
        preparing || emos_key_faulted || !uart1_keyboard_owned) {
        emos_keyboard_unlock(irq); return EMOS_KEY_BUSY;
    }
    /* Private fixed-size telemetry envelope: copy once into the UART owner's
     * resident slot. Gateway validation establishes the 140-byte input bound. */
    memcpy(async_data,"\x17\x00\xF5\x8C",4);
    memcpy(async_data+4,data,140);
    transitioning=1; async_position=0; async_at=emos_keyboard_clock();
    async_length=sizeof(async_data); /* Publish complete Core copy before IRQ enable. */
    uart1_keyboard_tx_enable(1);
    emos_keyboard_unlock(irq);
    return EMOS_KEY_OK;
}

#endif
