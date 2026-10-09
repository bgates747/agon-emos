/* Resident keyboard ingress; private EMOS interfaces, not a new MOS API. */
#ifndef EMOS_KEYBOARD_H
#define EMOS_KEYBOARD_H
#include <defines.h>

#define EMOS_KEY_MAINBOARD 0
#define EMOS_KEY_BROWSER 1
#define EMOS_KEY_EXTENDER 2
#define EMOS_KEY_OK 0
#define EMOS_KEY_BUSY 1
#define EMOS_KEY_TIMEOUT 2
#define EMOS_KEY_FAULT 3
/* Stock MOS v3.0.2 keyboard_lookup has 248 entries (virtual codes 1..248).
 * The linked keyboard check binds this bound to that table. Zero is valid. */
#define EMOS_KEY_MAX 248

extern volatile BYTE emos_key_source, emos_key_faulted;
BYTE emos_keyboard_select(BYTE source);
BYTE emos_keyboard_transport_claim(void);
/* Private handover leaves, not an ExExt command/API. Caller owns admission,
 * peer quiescence, handover signals, deadline and recovery (PORT-008). */
/* Reserve before the offer. Successful unpark keeps this reservation; release
 * only after the coordinator's status/recovery decision, with UART restored.
 * Private send reuses ordinary bounded TX and rejects recursive callbacks. */
BYTE emos_keyboard_parallel_reserve(void);
BYTE emos_keyboard_parallel_reserved(void);
BYTE emos_keyboard_parallel_send(const BYTE *data, UINT16 length);
BYTE emos_keyboard_parallel_release(void);
BYTE emos_keyboard_parallel_park(void);
BYTE emos_keyboard_parallel_unpark(void);
void emos_keyboard_transport_release(void);
BYTE emos_keyboard_send(const BYTE *data, UINT16 length);
/* BENCH-001: copy one bounded packet; ISR owner drains it without waiting. */
BYTE emos_keyboard_queue_telemetry(const BYTE *snapshot);
void emos_keyboard_async_irq(void);
BYTE emos_keyboard_text(const BYTE *text, UINT16 length);
BYTE emos_keyboard_layout(BYTE layout);
void emos_keyboard_byte(BYTE value);
void emos_keyboard_fault(void);
void emos_keyboard_tick(void);
void emos_keyboard_mainboard(BYTE *payload);
void emos_keyboard_mainboard_settings(BYTE *payload);

/* Private reusable foreground deadline. Same fields/clock/fuse as UART TX;
 * startup release reuses it instead of adding a second timing policy. */
typedef struct { BYTE last; UINT16 elapsed; UINT24 budget; } t_emosDeadline;
BYTE emos_keyboard_deadline_step(t_emosDeadline *d);

/* Target boundary. IRQ callers have all registers saved and interrupts off.
 * The foreground never invokes effects/cleanup: the stock callback is an ISR. */
BYTE emos_keyboard_clock(void);
BYTE emos_keyboard_lock(void);
void emos_keyboard_unlock(BYTE enabled);
void emos_keyboard_effect(BYTE *payload);
void emos_keyboard_settings(BYTE *payload);
void emos_keyboard_mainboard_layout(BYTE layout);

#endif
