; INTEG-021: Tom Morton ffab9c9 TX, MIT upstream Eirene source.
; INTEG-024 bounded CTS derivative: UART owner05b8b10, donor97311c43.
; One CTS sample before source/count mutation; defer AF/EI/RETI unchanged.
; VBlank rearm only, no resident copier, new timer or callback ABI.
; Deferred pending is live ownership even with TXIER off and TEMT set.
; Actual reset BSS clears retry; no route/abort shortcut cancels borrowed storage.
; RX dispatch/saves remain in interrupts.asm. No physical latency/cure claim.
; Public25 preserves BC/DE/HL/IX/IY/shadows/MB, returns A result, flags unspecified.
; Rejects entry-IFF0 before DI; no callback/descriptor/IER mutation on refusal.
.assume adl=1
.include "equs.inc"
.text
.global emos_uart0_rfc25_submit,fill_uart0_tx_fifo,_emos_uart0_async_owned
.global uart0_async_retry,uart0_tx_retry_pending
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
 xor a
 ld (uart0_tx_retry_pending),a
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
 ld a,(uart0_tx_retry_pending)
 or a
 jr nz,rfc25_busy
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

; Interrupt-driven UART0 transmission
fill_uart0_tx_fifo:
            in0 a,(PD_DR)
            tst a,8
            jr nz,uart0_tx_defer
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
			otirx

			; disable interrupt since send is finished
			in0 a,(UART0_IER)
			and 0xfd
			out0 (UART0_IER),a

			xor a
			ld (uart0_tx_retry_pending),a
			call uart0_tx_callback
			pop hl
			pop de
			pop bc
			pop af	; saved by _uart0_handler
			ei
			reti.lil
uart0_tx_defer:
            in0 a,(UART0_IER)
            and 0xfd
            out0 (UART0_IER),a
            ld a,1
            ld (uart0_tx_retry_pending),a
            pop af
            ei
            reti.lil

; Called under existing VBlank exclusion/saves. No UART I/O while inactive.
; Rearm only: the UART0 interrupt retains sole ownership of FIFO copying.
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
            or 2
            out0 (UART0_IER),a
            ret

.bss
uart0_tx_retry_pending: .ds 1
uart0_tx_data_pos: .ds 3
uart0_tx_data_count: .ds 3
uart0_tx_callback: .ds 5
.global _emos_rfc25_owned
_emos_rfc25_owned: .ds 1
