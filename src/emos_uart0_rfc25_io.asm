; INTEG-021: Tom Morton ffab9c9 TX/WAIT_CTS, MIT upstream Eirene source.
; Exact TX leaf below; ADL-only admission makes Z80 thunk construction unused.
; RX dispatch/saves remain in interrupts.asm. WAIT_CTS under DI is intentionally
; unbounded per selected RFC; it does not promise bounded service latency.
; Public25 preserves BC/DE/HL/IX/IY/shadows/MB, returns A result, flags unspecified.
; Rejects entry-IFF0 before DI; no callback/descriptor/IER mutation on refusal.
.assume adl=1
.include "equs.inc"
.text
.global emos_uart0_rfc25_submit,fill_uart0_tx_fifo,_emos_uart0_async_owned
.extern _emos_rfc25_admit
emos_uart0_rfc25_submit:
 ld a,i
 jp po,rfc25_unsafe
 ld a,mb
 or a
 jr nz,rfc25_unsafe
 di
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
 ld a,(_emos_rfc25_owned)
 or a
 ret z
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

.macro WAIT_CTS
1:
 in0 a,(PD_DR)
 tst a,8
 jr nz,1b
.endm

; Interrupt-driven UART0 transmission
fill_uart0_tx_fifo:
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
			WAIT_CTS
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
			WAIT_CTS
			otirx

			; disable interrupt since send is finished
			in0 a,(UART0_IER)
			and 0xfd
			out0 (UART0_IER),a

			call uart0_tx_callback
			pop hl
			pop de
			pop bc
			pop af	; saved by _uart0_handler
			ei
			reti.lil
.bss
uart0_tx_data_pos: .ds 3
uart0_tx_data_count: .ds 3
uart0_tx_callback: .ds 5
.global _emos_rfc25_owned
_emos_rfc25_owned: .ds 1
