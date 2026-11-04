; Unpublished TR-001 trial, based on Tom Morton and the released RC1.
; Normal MOS saves retained. Preconditions/callback convention: TR-001 contract.
.assume adl=1
include "equs.inc"
.text
.global uart0_async_write,uart0_async_finish,uart0_async_isr
.extern _uart0_handler,__2nd_jump_table,_emosVduBackend
.global _emos_uart0_async_owned
.equ async_vector,__2nd_jump_table+0x31
UART0_ASYNC_SCALAR equ 0

; Foreground ADL1/MBASE0/EI, normal UART setup, no competing owner.
; Preserve HL/BC/DE/IX/IY/shadows. BC and indexes are not touched here.
uart0_async_write:
 di
 push hl
 push de
 ; EMOS-001: UART0 is the onboard output only; ExCom must not bypass route.
 ld a,(_emosVduBackend)
 or a
 jr nz,async_write_busy
 ld hl,(async_left)
 call async_zero
 jr nz,async_write_busy
 ld hl,0
 or a
 sbc hl,bc
 jr z,async_write_invalid
 pop de
 pop hl
 push hl
 ld (async_source),hl
 ld (async_left),bc
 ld (async_callback),de
 ; Publish complete state and vector before enabling UART transmit IRQs.
 ld hl,uart0_async_isr
 ld (async_vector),hl
 in0 a,(UART0_IER)
 or 0x0a
 out0 (UART0_IER),a
 pop hl
 xor a
 ei
 ret
async_write_busy:
 ld a,1
 jr async_write_rejected
async_write_invalid:
 ld a,2
async_write_rejected:
 pop de
 pop hl
 ei
 ret

; Nonblocking, also safe before first submit under the same ownership contract.
uart0_async_finish:
 di
 push hl
 push de
 ld hl,(async_left)
 call async_zero
 jr nz,async_finish_busy
 in0 a,(UART0_LSR)
 bit 6,a
 jr z,async_finish_busy
 in0 a,(UART0_IER)
 and 0xf5                 ; Release only our TX-empty/modem enables.
 out0 (UART0_IER),a
 ld hl,_uart0_handler
 ld (async_vector),hl
 xor a
 jr async_finish_return
async_finish_busy:
 ld a,1
async_finish_return:
 pop de
 pop hl
 ei
 ret

; Full24-bit zero test; DE is scratch and saved by each caller's boundary.
async_zero:
 ld de,0
 or a
 sbc hl,de
 ret

; Chain RX to the original MOS ISR, including its ordinary keyboard callback.
; Handle only TX-empty/CTS here. One entry fills at most one FIFO; never spins.
uart0_async_isr:
 di
 push af
 in0 a,(UART0_IIR)
 bit 0,a
 jr nz,async_irq_done
 and 0x0e
 cp 2
 jr z,async_tx_irq
 or a
 jr z,async_modem_irq
 cp 6
 jr nz,async_chain_rx
 ; A status-only interrupt has no byte to inject into the MOS state machine.
 in0 a,(UART0_LSR)
 bit 0,a
 jr z,async_irq_done
async_chain_rx:
 pop af
 jp _uart0_handler
async_modem_irq:
 in0 a,(UART0_MSR)          ; Acknowledge modem cause even if FIFO is still busy.
async_tx_irq:
 push bc
 push de
 push hl
 call async_fill
 pop hl
 pop de
 pop bc
async_irq_done:
 pop af
 ei
 reti.l

async_fill:
 ld hl,(async_left)
 call async_zero
 ret z
 in0 a,(UART0_LSR)
 bit 5,a
 ret z
 in0 a,(UART0_IER)
 or 0x0a                   ; Arm CTS wakeup BEFORE sampling its level.
 out0 (UART0_IER),a
 in0 a,(UART0_MSR)
 bit 4,a
 jr z,async_cts_blocked
 ld bc,16
 or a
 sbc hl,bc
 jr nc,async_full_block
 add hl,bc
 push hl
 pop bc
async_full_block:
 push bc
 ld hl,(async_source)
 ld de,UART0_THR
.if UART0_ASYNC_SCALAR
async_scalar_copy:
 ld a,(hl)
 out0 (UART0_THR),a
 inc hl
 dec bc
 ld a,b
 or c
 jr nz,async_scalar_copy
.else
 otirx
.endif
 ld (async_source),hl
 pop bc
 ld hl,(async_left)
 or a
 sbc hl,bc
 ld (async_left),hl
 ret nz
 in0 a,(UART0_IER)
 and 0xf5
 out0 (UART0_IER),a
 ld hl,(async_callback)
 call async_zero
 ret z
 ; Stock-style callback: preserve indexes/shadows in an app wrapper if touched.
 ; The ISR retains AF/BC/DE/HL until this callback returns directly to it.
 xor a
 jp (hl)
async_cts_blocked:
 in0 a,(UART0_IER)
 and 0xfd                      ; Mask level-triggered TX-empty while CTS is blocked.
 out0 (UART0_IER),a
 in0 a,(UART0_MSR)          ; Do not lose a rising CTS edge during the mask.
 bit 4,a
 ret z
 in0 a,(UART0_IER)
 or 0x0a
 out0 (UART0_IER),a
 ret                       ; Re-arm next entry, not a DI retry loop.

; BYTE result in A. Reuse vector ownership, including source-released/wire-tail
; phase, rather than allocate another resident session flag.
_emos_uart0_async_owned:
 ld hl,(async_vector)
 ld de,uart0_async_isr
 or a
 sbc hl,de
 ld a,0
 ret nz
 inc a
 ret

.bss
async_source: .ds 3
async_left: .ds 3
async_callback: .ds 3
