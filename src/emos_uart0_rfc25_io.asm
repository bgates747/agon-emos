; INTEG-021: Tom Morton ffab9c9 TX/WAIT_CTS, MIT upstream Eirene source.
; Exact TX leaf below; ADL-only admission makes Z80 thunk construction unused.
; RX saves remain in interrupts.asm. The legacy option waits under DI.
; The PARLIO CTS-IRQ option defers a blocked refill and returns to the caller.
; Donor: INTEG-024 bounded refill and INTEG-028 modem-edge rearm.
; Public25 preserves BC/DE/HL/IX/IY/shadows/MB, returns A result, flags unspecified.
; Rejects entry-IFF0 before DI; no callback/descriptor/IER mutation on refusal.
.assume adl=1
.include "equs.inc"
.text
.global emos_uart0_rfc25_submit,fill_uart0_tx_fifo,_emos_uart0_async_owned
.extern _emos_rfc25_admit
.ifdef EMOS_UART0_CTS_IRQ
.global uart0_async_retry,uart0_cts_irq,uart0_tx_retry_pending
.endif
emos_uart0_rfc25_submit:
 ld a,i
 jp po,rfc25_unsafe
 ld a,mb
 or a
 jr nz,rfc25_unsafe
 di
.ifdef EMOS_UART0_CTS_IRQ
 ; Borrow modem-enable only when initially clear. Refusal changes no descriptor.
 in0 a,(UART0_IER)
 tst a,8
 jr z,rfc25_cts_available
 ld a,31
 ei
 ret
rfc25_cts_available:
.endif
 push hl
 push bc
 push de
 push de
 push bc
 push hl
 call _emos_rfc25_admit
 pop hl
 pop bc
 pop de
 or a
 jr nz,rfc25_return
.ifdef EMOS_UART0_CTS_IRQ
 xor a
 ld (uart0_tx_retry_pending),a
.endif
 ld (uart0_tx_data_pos),hl
 ld (uart0_tx_data_count),bc
 ; Tom ADL callback thunk: CALL full24-bit address; RET to TX ISR.
 ld hl,uart0_tx_callback
 ld (hl),0xcd
 inc hl
 ld (hl),de
 inc hl
 inc hl
 inc hl
 ld (hl),0xc9
 in0 a,(UART0_IER)
 or 2
 out0 (UART0_IER),a
 xor a
rfc25_return:
 pop de
 pop bc
 pop hl
 ei
 ret
rfc25_unsafe:
 ld a,32
 ret

; C ABI result A: never wait or require another IRQ to retire the FIFO tail.
_emos_uart0_async_owned:
.ifdef EMOS_UART0_CTS_IRQ
 ; IRQ can change pending and IER together. Restore the caller's IFF.
 push bc
 ld a,i
 di
 push af
 call rfc25_owned_locked
 ld c,a
 pop af
 ld a,c
 pop bc
 ret po
 ei
 ret
rfc25_owned_locked:
.endif
 ld a,(_emos_rfc25_owned)
 or a
 ret z
.ifdef EMOS_UART0_CTS_IRQ
 ld a,(uart0_tx_retry_pending)
 or a
 jr nz,rfc25_busy
.endif
 in0 a,(UART0_IER)
 bit 1,a
 jr nz,rfc25_busy
 in0 a,(UART0_LSR)
 bit 6,a
 jr z,rfc25_busy
 xor a
 ld (_emos_rfc25_owned),a
 ret
rfc25_busy:
 ld a,1
 ret

.ifndef EMOS_UART0_CTS_IRQ
.macro WAIT_CTS
1:
 in0 a,(PD_DR)
 tst a,8
 jr nz,1b
.endm
.endif

; Interrupt-driven UART0 transmission
fill_uart0_tx_fifo:
.ifdef EMOS_UART0_CTS_IRQ
 ; Sample CTS before touching the count or position. Once admitted, retain
 ; the existing complete FIFO burst and then sample again on the next IRQ.
 in0 a,(PD_DR)
 tst a,8
 jp nz,uart0_tx_defer
.endif
			push bc
			push de
			push hl

			ld bc,16
			ld hl,(uart0_tx_data_count)
			or a
			sbc hl,bc

			jr z,.last_fill
			jr c,.last_fill

			; send 16 bytes
			ld (uart0_tx_data_count),hl
			ld hl,(uart0_tx_data_pos)
			ld de,UART0_THR
.ifndef EMOS_UART0_CTS_IRQ
			WAIT_CTS
.endif
			otirx
			ld (uart0_tx_data_pos),hl
			pop hl
			pop de
			pop bc
			pop af	; saved by _uart0_handler
			ei
			reti.lil

		.last_fill:
			; send <=16 final bytes
			ld bc,(uart0_tx_data_count)
			ld hl,(uart0_tx_data_pos)
			ld de,UART0_THR
.ifndef EMOS_UART0_CTS_IRQ
			WAIT_CTS
.endif
			otirx

			; disable interrupt since send is finished
			in0 a,(UART0_IER)
			and 0xfd
			out0 (UART0_IER),a

.ifdef EMOS_UART0_CTS_IRQ
            xor a
            ld (uart0_tx_retry_pending),a
.endif
			call uart0_tx_callback
			pop hl
			pop de
			pop bc
			pop af	; saved by _uart0_handler
			ei
			reti.lil
.ifdef EMOS_UART0_CTS_IRQ
uart0_tx_defer:
 ; Publish pending before arm, then acknowledge and resample so a ready
 ; edge during the transition is observed or remains latched.
 ld a,1
 ld (uart0_tx_retry_pending),a
 in0 a,(UART0_IER)
 and 0xfd
 or 8
 out0 (UART0_IER),a
 in0 a,(UART0_MSR)
 call uart0_async_retry
 pop af
 ei
 reti.lil

uart0_cts_irq:
 in0 a,(UART0_MSR)
 call uart0_async_retry
 pop af
 ei
 reti.lil

; Called by the modem handler and VBlank with IRQs already excluded.
; Rearm only. The TX IRQ owns all copies and the source-release callback.
uart0_async_retry:
 ld a,(uart0_tx_retry_pending)
 or a
 ret z
 in0 a,(PD_DR)
 tst a,8
 ret nz
 xor a
 ld (uart0_tx_retry_pending),a
 in0 a,(UART0_IER)
 and 0xf7
 or 2
 out0 (UART0_IER),a
 ret
.endif

.bss
.ifdef EMOS_UART0_CTS_IRQ
uart0_tx_retry_pending: .ds 1
.endif
uart0_tx_data_pos: .ds 3
uart0_tx_data_count: .ds 3
uart0_tx_callback: .ds 5
.global _emos_rfc25_owned
_emos_rfc25_owned: .ds 1
