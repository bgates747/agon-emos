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
void emos_keyboard_transport_release(void);
BYTE emos_keyboard_send(const BYTE *data, UINT16 length);
BYTE emos_keyboard_text(const BYTE *text, UINT16 length);
BYTE emos_keyboard_layout(BYTE layout);
void emos_keyboard_byte(BYTE value);
void emos_keyboard_fault(void);
void emos_keyboard_tick(void);
void emos_keyboard_mainboard(BYTE *payload);
void emos_keyboard_mainboard_settings(BYTE *payload);

/* Target boundary. IRQ callers have all registers saved and interrupts off.
 * The foreground never invokes effects/cleanup: the stock callback is an ISR. */
BYTE emos_keyboard_clock(void);
BYTE emos_keyboard_lock(void);
void emos_keyboard_unlock(BYTE enabled);
void emos_keyboard_effect(BYTE *payload);
void emos_keyboard_settings(BYTE *payload);
void emos_keyboard_mainboard_layout(BYTE layout);

#endif
