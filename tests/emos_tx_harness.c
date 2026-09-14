/* INTEG-014: real sender with controlled hardware-boundary outcomes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emos_keyboard.h"
#include "uart.h"
volatile BYTE uart1_keyboard_owned, emos_console_owned;
static BYTE irq_enabled=1, in_irq, frozen_clock, clock_byte;
static BYTE reply, pending_reply, tx_blocked, rx_stopped;
static BYTE tx[256];
static unsigned tx_count;
void emos_telemetry_reset(void) {}
void emos_sdlink_reset(void) {}
void emos_sdlink_packet(const BYTE *p, BYTE n) {(void)p;(void)n;}
void emos_console_packet(BYTE c, BYTE *p, BYTE n) {(void)c;(void)p;(void)n;}
void uart1_keyboard_tx_enable(BYTE enabled) {(void)enabled;}
BYTE emos_keyboard_lock(void) {BYTE old=irq_enabled;irq_enabled=0;return old;}
void emos_keyboard_unlock(BYTE enabled) {irq_enabled=enabled;}
static void tick(void) {BYTE old=in_irq;in_irq=1;emos_keyboard_tick();in_irq=old;}
static BYTE receiver_clock(void) {
    if (!in_irq && !frozen_clock) {
        clock_byte+=2;
        if (irq_enabled) {
            tick();
            if (pending_reply) {
                BYTE p[]={0x80,1,tx[tx_count-1]}; unsigned i;
                pending_reply=0;in_irq=1;
                for(i=0;i<sizeof(p);++i)emos_keyboard_byte(p[i]);
                in_irq=0;
            }
        }
    }
    return clock_byte;
}
BYTE uart1_keyboard_open(void) {
    assert(!irq_enabled);uart1_keyboard_owned=1;rx_stopped=0;tx_count=0;
    return UART_POLL_READY;
}
void uart1_keyboard_stop(void) {if(uart1_keyboard_owned)rx_stopped=1;}
void uart1_keyboard_close(void) {assert(!irq_enabled);uart1_keyboard_owned=0;}
static BYTE receiver_put(BYTE value) {
    if(tx_blocked)return UART_POLL_BLOCKED;
    assert(tx_count<sizeof(tx));tx[tx_count++]=value;
    if(tx_count>=4 && tx[tx_count-4]==23 && tx[tx_count-3]==0 && tx[tx_count-2]==0x80 && reply)pending_reply=1;
    return UART_POLL_READY;
}
void emos_keyboard_effect(BYTE *p) {(void)p;assert(in_irq);}
void emos_keyboard_settings(BYTE *p) {(void)p;assert(in_irq);}
void emos_keyboard_mainboard_layout(BYTE value) {(void)value;}
static void activate(void) {
    assert(emos_keyboard_select(EMOS_KEY_BROWSER)==EMOS_KEY_OK);
    assert(!emos_key_faulted && uart1_keyboard_owned);
}

static unsigned attempts, fail_after, clock_reads;
static BYTE *mutate_during_wait;
static BYTE injected_result, lose_owner, inject_fault, busy_result, busy_enabled;
BYTE emos_keyboard_clock(void) { ++clock_reads; return receiver_clock(); }
BYTE uart1_keyboard_put(BYTE value) {
    ++attempts;
    if (mutate_during_wait) {
        *mutate_during_wait=0xEE;mutate_during_wait=NULL;return UART_POLL_BLOCKED;
    }
    if (fail_after && attempts > fail_after) {
        if (lose_owner) uart1_keyboard_owned = 0;
        if (inject_fault) { in_irq=1; emos_keyboard_fault(); in_irq=0; }
        return injected_result;
    }
    if (busy_enabled) return busy_result;
    return receiver_put(value);
}
static void fresh(void) {
    fail_after=0;lose_owner=0;inject_fault=0;busy_result=0;busy_enabled=0;tx_blocked=0;
    frozen_clock=0;irq_enabled=1;reply=1;
    assert(emos_keyboard_select(EMOS_KEY_MAINBOARD)==EMOS_KEY_OK);
    activate();reply=0;tx_count=0;attempts=0;clock_reads=0;
}
int main(void) {
    BYTE data[]={0,0x81,4,0xFF,23,0,0x80,1};
    unsigned n;
    fresh();
    assert(emos_keyboard_send(data,sizeof(data))==EMOS_KEY_OK);
    assert(tx_count==sizeof(data) && !memcmp(tx,data,sizeof(data)) && irq_enabled);
    assert(emos_keyboard_send(data,0)==EMOS_KEY_OK && tx_count==sizeof(data));
    irq_enabled=0;
    assert(emos_keyboard_send(data,1)==EMOS_KEY_BUSY && !irq_enabled);
    irq_enabled=1;uart1_keyboard_owned=0;
    assert(emos_keyboard_send(data,1)==EMOS_KEY_BUSY);
    uart1_keyboard_owned=1;
    for(n=0;n<4;++n) {
        fresh();fail_after=3;
        injected_result=n==0?UART_POLL_ERROR:n==1?UART_POLL_UNAVAILABLE:UART_POLL_BLOCKED;
        lose_owner=n==1;inject_fault=n==2;frozen_clock=n==3;
        assert(emos_keyboard_send(data,sizeof(data))==EMOS_KEY_TIMEOUT);
        assert(tx_count==3 && !memcmp(tx,data,3) && irq_enabled);
        assert(attempts<=262144U+3U);
        /* The established deferred cleanup contract runs at the next IRQ. */
        tick();assert(lose_owner ? !emos_key_faulted : (emos_key_faulted && rx_stopped));
        assert(emos_keyboard_send(data,sizeof(data))==EMOS_KEY_BUSY && tx_count==3);
        uart1_keyboard_owned=1;
    }
    for(n=0;n<2;++n) {
        fresh();busy_enabled=1;busy_result=n?UART_POLL_EMPTY:UART_POLL_BLOCKED;
        assert(emos_keyboard_send(data,1)==EMOS_KEY_TIMEOUT && tx_count==0);
        assert(attempts==299 && irq_enabled); /* 600 clock units, whole call. */
        tick();assert(emos_key_faulted);
        fresh();busy_enabled=1;busy_result=n?UART_POLL_EMPTY:UART_POLL_BLOCKED;frozen_clock=1;
        assert(emos_keyboard_send(data,1)==EMOS_KEY_TIMEOUT && tx_count==0);
        assert(attempts==262143U && clock_reads==262145U && irq_enabled);
        tick();assert(emos_key_faulted);
    }
    fresh();
    data[0]=0x42;mutate_during_wait=data;
    assert(emos_keyboard_send(data,1)==EMOS_KEY_OK && tx_count==1 && tx[0]==0x42);
    fresh();clock_byte=254;
    assert(emos_keyboard_send(data,sizeof(data))==EMOS_KEY_OK && tx_count==sizeof(data));
    puts("TX partial/error/ownership/IRQ/clock-wrap/stall boundaries passed");
    return 0;
}
