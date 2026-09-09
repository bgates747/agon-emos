/* Execute the maintained receiver, replacing only hardware/assembly effects. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emos_keyboard.h"
#include "uart.h"

volatile BYTE uart1_keyboard_owned;
static BYTE irq_enabled = 1, in_irq, clock_byte, frozen_clock;
static BYTE occupied, reply, tx_error, rx_stopped, edit_key, interleave;
static BYTE tx[64], tx_count, pending_reply;
static BYTE events[600][4], settings[5], mainboard_layout;
static unsigned event_count, settings_count, closed;

static void frame(const BYTE *p, unsigned count) {
    BYTE old = in_irq; in_irq = 1;
    while (count--) emos_keyboard_byte(*p++);
    in_irq = old;
}
static void key(BYTE code, BYTE mods, BYTE vk, BYTE down) {
    BYTE p[] = {0x81,4,code,mods,vk,down}; frame(p, sizeof(p));
}
static void tick(void) {
    BYTE old = in_irq; in_irq = 1; emos_keyboard_tick(); in_irq = old;
}
BYTE emos_keyboard_clock(void) {
    if (!in_irq && !frozen_clock) {
        clock_byte += 2;
        if (irq_enabled) {
            tick();
            if (pending_reply) {
                BYTE p[] = {0x80,1,tx[tx_count-1]};
                pending_reply = 0;
                if (reply == 2) p[2]++;
                frame(p,3);
            }
        }
    }
    return clock_byte;
}
BYTE emos_keyboard_lock(void) { BYTE old = irq_enabled; irq_enabled = 0; return old; }
void emos_keyboard_unlock(BYTE enabled) { irq_enabled = enabled; }
BYTE uart1_keyboard_open(void) {
    if (occupied) return UART_POLL_UNAVAILABLE;
    assert(!irq_enabled); uart1_keyboard_owned = 1; rx_stopped = 0; tx_count = 0;
    return UART_POLL_READY;
}
void uart1_keyboard_stop(void) { if (uart1_keyboard_owned) rx_stopped = 1; }
void uart1_keyboard_close(void) {
    assert(!irq_enabled);
    if (uart1_keyboard_owned) { uart1_keyboard_owned = 0; closed++; }
}
BYTE uart1_keyboard_put(BYTE value) {
    if (tx_error) return UART_POLL_ERROR;
    assert(tx_count < sizeof(tx)); tx[tx_count++] = value;
    if (tx_count>=4 && tx[tx_count-4]==23 && tx[tx_count-3]==0 && tx[tx_count-2]==0x80 && reply) {
        if (interleave) key('q',0,38,1);
        pending_reply = 1;
    }
    return UART_POLL_READY;
}
void emos_keyboard_effect(BYTE *p) {
    assert(in_irq);
    if (edit_key) { p[0] = 'Z'; p[2] = edit_key; }
    assert(event_count < 600);
    memcpy(events[event_count++], p, 4);
}
void emos_keyboard_settings(BYTE *p) { assert(in_irq); memcpy(settings,p,5); settings_count++; }
void emos_keyboard_mainboard_layout(BYTE p) { mainboard_layout = p; }
static void mainboard_key(BYTE code, BYTE mods, BYTE vk, BYTE down) {
    BYTE p[] = {code,mods,vk,down}; in_irq = 1; emos_keyboard_mainboard(p); in_irq = 0;
}
static void activate(void) {
    reply = 1; tx_error = 0; frozen_clock = 0;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_OK);
    assert(emos_key_source == EMOS_KEY_BROWSER && !emos_key_faulted && uart1_keyboard_owned);
}
int main(void) {
    unsigned before, count;
    BYTE p[260];
    assert(!emos_key_source && !uart1_keyboard_owned);
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD) == EMOS_KEY_OK);
    mainboard_key('a',0,20,1); assert(event_count == 1);
    occupied = 1;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_BUSY);
    assert(!emos_key_source && event_count == 1 && !uart1_keyboard_owned && !closed);
    occupied = 0; reply = 0;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_TIMEOUT);
    assert(!emos_key_source && !uart1_keyboard_owned && event_count == 1);
    frozen_clock = 1;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_TIMEOUT);
    frozen_clock = 0; reply = 2;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_TIMEOUT);
    assert(!emos_key_source);
    assert(emos_keyboard_layout(1) == EMOS_KEY_OK && mainboard_layout == 1);
    activate();
    assert(tx_count == 8 && !memcmp(tx, (BYTE[]){23,0,0x81,1,23,0,0x80}, 7));
    assert(event_count == 2 && events[1][2] == 20 && !events[1][3]);
    before = event_count;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_OK && tx_count == 8);
    mainboard_key('b',0,21,1); assert(event_count == before);
    in_irq = 1; emos_keyboard_mainboard_settings((BYTE[]){1,2,3,4,5}); in_irq = 0;
    assert(!settings_count);
    frame((BYTE[]){0x88,5,1,2,3,4,5},7); assert(settings_count == 1 && settings[4] == 5);
    /* Fragmentation and header-looking payload are ordinary data. */
    frame((BYTE[]){0x81},1); frame((BYTE[]){4,0x80,0},3);
    assert(event_count == before); frame((BYTE[]){22,1},2);
    assert(event_count == before+1 && events[before][0] == 0x80);
    p[0] = 0xFE; p[1] = 255; memset(p+2,0x81,255); frame(p,257);
    assert(event_count == before+1 && !emos_key_faulted);
    frame((BYTE[]){0xF0,0,0x80,1,0x88},5); assert(!emos_key_faulted);
    key('c',0,23,1); key('c',0,23,1); key('c',0,23,0);
    assert(event_count == before+4);
    /* Track callback-edited vkey, then release it once on source change. */
    edit_key = 24; key('x',0,25,1); edit_key = 0;
    before = event_count;
    assert(emos_keyboard_layout(2) == EMOS_KEY_OK);
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD) == EMOS_KEY_OK);
    assert(!uart1_keyboard_owned && !emos_key_source && mainboard_layout == 2);
    assert(event_count == before+2 && events[event_count-1][2] == 24);
    before = event_count;
    key('x',0,25,1); assert(event_count == before);
    /* Modifiers first; no synthetic Ctrl+Alt+Delete combination. */
    mainboard_key(0,1,121,1); mainboard_key(0,5,119,1); mainboard_key(0,5,130,1);
    before = event_count; activate();
    assert(event_count == before+3);
    assert(events[before][2] == 119 && events[before+1][2] == 121 && events[before+2][2] == 130);
    for (count=before; count<event_count; ++count) assert(!(events[count][1]&5) && !events[count][3]);
    key('d',0,26,1); before = event_count;
    frame((BYTE[]){0x81,3},2);
    assert(emos_key_faulted && rx_stopped && emos_key_source == EMOS_KEY_BROWSER);
    assert(event_count == before+1 && !events[before][3]);
    tick(); key('d',0,26,1); assert(event_count == before+1);
    activate(); frame((BYTE[]){0x81,4,0,0,249,1},6); assert(emos_key_faulted);
    activate(); frame((BYTE[]){0x81,4,0,0,0,2},6); assert(emos_key_faulted);
    activate(); frame((BYTE[]){0x88,6},2); assert(emos_key_faulted);
    activate(); frame((BYTE[]){0x81,4,0},3);
    clock_byte += 28; tick(); assert(!emos_key_faulted);
    clock_byte += 2; tick(); assert(emos_key_faulted);
    /* Idle remains healthy across wraps; zero virtual code is valid. */
    activate(); before = event_count; key(0,0,0,1); assert(event_count == before+1);
    for(count=0; count<300; ++count) { clock_byte += 2; tick(); }
    assert(!emos_key_faulted);
    tx_error = 1; assert(emos_keyboard_layout(7) == EMOS_KEY_TIMEOUT); tick();
    assert(emos_key_faulted); tx_error = 0;
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD) == EMOS_KEY_OK);
    assert(mainboard_layout == 2 && !uart1_keyboard_owned);
    /* Virtual code zero still has a key-down sysvar/callback lifetime. */
    activate(); before = event_count; key(0,1,0,1);
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD) == EMOS_KEY_OK);
    assert(event_count == before+2 && !events[event_count-1][3] && !events[event_count-1][1]);
    /* A stopped clock prevents a switch requiring IRQ cleanup, boundedly. */
    activate(); frozen_clock = 1;
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD) == EMOS_KEY_TIMEOUT);
    assert(emos_key_source == EMOS_KEY_BROWSER && uart1_keyboard_owned);
    frozen_clock = 0;
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD) == EMOS_KEY_OK);
    irq_enabled = 0;
    assert(emos_keyboard_select(EMOS_KEY_BROWSER) == EMOS_KEY_BUSY);
    assert(!irq_enabled); irq_enabled = 1;
    activate(); before=event_count; interleave=1;
    assert(emos_keyboard_text((BYTE *)"hello",5)==EMOS_KEY_OK);
    assert(uart1_keyboard_owned && event_count==before+1 && events[before][0]=='q');
    assert(tx_count==20 && !memcmp(tx+8,"hello",5));
    assert(!memcmp(tx+13,(BYTE[]){23,0,0xCA,23,0,0x80},6));
    interleave=0; reply=2;
    assert(emos_keyboard_text((BYTE *)"x",1)==EMOS_KEY_TIMEOUT); tick();
    assert(emos_key_faulted && !events[event_count-1][3]);
    activate(); reply=0; frozen_clock=1;
    assert(emos_keyboard_text((BYTE *)"x",1)==EMOS_KEY_TIMEOUT);
    frozen_clock=0; tick(); assert(emos_key_faulted);
    activate(); irq_enabled=0;
    assert(emos_keyboard_text((BYTE *)"x",1)==EMOS_KEY_BUSY);
    irq_enabled=1;
    /* Native USB selects the same IRQ-owned stock path under its own name. */
    assert(emos_keyboard_select(EMOS_KEY_EXTENDER)==EMOS_KEY_BUSY);
    assert(emos_key_source==EMOS_KEY_BROWSER && uart1_keyboard_owned);
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD)==EMOS_KEY_OK);
    reply=0;
    assert(emos_keyboard_select(EMOS_KEY_EXTENDER)==EMOS_KEY_TIMEOUT);
    assert(emos_key_source==EMOS_KEY_MAINBOARD && !uart1_keyboard_owned);
    reply=1;
    assert(emos_keyboard_select(EMOS_KEY_EXTENDER)==EMOS_KEY_OK);
    assert(emos_key_source==EMOS_KEY_EXTENDER && uart1_keyboard_owned);
    before=event_count;
    mainboard_key('x',0,45,1); assert(event_count==before);
    key('a',0,22,1); assert(event_count==before+1);
    assert(emos_keyboard_text((BYTE *)"x",1)==EMOS_KEY_BUSY);
    assert(emos_keyboard_layout(1)==EMOS_KEY_OK);
    assert(emos_key_source==EMOS_KEY_EXTENDER && !emos_key_faulted);
    frame((BYTE[]){0x81,3},2);
    assert(emos_key_faulted && events[event_count-1][2]==22 && !events[event_count-1][3]);
    assert(emos_keyboard_select(EMOS_KEY_EXTENDER)==EMOS_KEY_OK);
    key('z',0,47,1); before=event_count;
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD)==EMOS_KEY_OK);
    assert(event_count==before+1 && !events[before][3] && !uart1_keyboard_owned);
    /* Native layout is committed only after its ordered readiness reply. */
    assert(emos_keyboard_select(EMOS_KEY_EXTENDER)==EMOS_KEY_OK);
    key('a',0,22,1); reply=0;
    assert(emos_keyboard_layout(2)==EMOS_KEY_TIMEOUT); tick();
    assert(emos_key_faulted && !events[event_count-1][3]);
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD)==EMOS_KEY_OK);
    assert(mainboard_layout==1); /* Failed layout never replaces retained US. */
    puts("resident keyboard parser, admission, ownership and IRQ cleanup scenarios passed");
    return 0;
}
