; Single-buffer UART0 sender, based on Tom Morton's async_vdu.asm.
; See docs/tasks/TM-001/reference/async_vdu.asm and contract.md for provenance.
; No startup hook, payload copy, queue, ticket or change to RST18.
.assume adl=1
include "equs.inc"
.text
.global uart0_async_write,uart0_async_finish,uart0_async_isr
.extern _uart0_handler,__2nd_jump_table,_serialFlags
UART0_ASYNC_SCALAR equ 0
.equ async_vector,__2nd_jump_table+0x31 ; UART0 vector18h *2 + JP operand

; Foreground ADL/MBASE0 only. Require EI; never wait or pump inside a caller ISR.
; Saved arguments at IX: callback+0, length+3, source+6.
.macro async_api name,core
\name:
 ld a,mb
 or a
 jp nz,async_context
 ld a,i
 jp po,async_context
 di
 push ix
 push hl
 push bc
 push de
 ld ix,0
 add ix,sp
 call \core
 pop de
 pop bc
 pop hl
 pop ix
 ei
 ret
.endm
async_api uart0_async_write,async_write_core
async_api uart0_async_finish,async_finish_core
async_context:
 ld a,3
 ret
async_busy:
 ld a,1
 ret
async_invalid:
 ld a,2
 ret
async_ok:
 xor a
 ret
async_zero:                  ; Full24-bit test; preserve HL/BC/DE.
 push de
 ld de,0
 or a
 sbc hl,de
 pop de
 ret

async_write_core:
 ld hl,(async_left)
 call async_zero
 jp nz,async_busy
 ld hl,(ix+3)
 call async_zero
 jp z,async_invalid
 ; Like RST18, pointer validity/lifetime is the trusted caller's contract.
 ld a,(async_enabled)
 or a
 jr nz,async_publish
 ; Refuse a competing UART owner; leave the baseline RX configuration intact.
 ld a,(_serialFlags)
 and 3
 cp 3
 jp nz,async_busy
 ld hl,(async_vector)
 ld de,_uart0_handler
 or a
 sbc hl,de
 jp nz,async_busy
 in0 a,(UART0_IER)
 and 0xfa                  ; Only baseline receive/line-status IRQs may be set.
 jp nz,async_busy
 in0 a,(UART0_LSR)
 bit 6,a
 jp z,async_busy            ; Do not activate across unfinished ordinary output.
 in0 a,(UART0_IER)
 ld (async_saved_ier),a
 ld a,1
 ld (async_enabled),a
async_publish:
 ld hl,(ix+6)
 ld (async_source),hl
 ld hl,(ix+3)
 ld (async_left),hl
 ld hl,(ix+0)
 ld (async_callback),hl
 ; Publish complete state AND install the vector under the same DI scope.
 ld hl,uart0_async_isr
 ld (async_vector),hl
 ld a,(async_saved_ier)
 or 0x0a                   ; TX-empty plus CTS-change wakeup.
 out0 (UART0_IER),a
 jp async_ok

async_finish_core:
 ld a,(async_enabled)
 or a
 ret z                     ; Inactive: no hardware access or vector changes.
 ld hl,(async_left)
 call async_zero
 jp nz,async_busy
 in0 a,(UART0_LSR)
 bit 6,a
 jp z,async_busy
 ld a,(async_saved_ier)
 out0 (UART0_IER),a
 ld hl,_uart0_handler
 ld (async_vector),hl
 xor a
 ld (async_enabled),a
 ld hl,0
 ld (async_callback),hl
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
 ld a,(async_saved_ier)
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
 ld a,(async_saved_ier)
 out0 (UART0_IER),a
 ld hl,(async_callback)
 call async_zero
 ret z
 ; Main registers are saved by the ISR. Protect indexes/shadow banks around
 ; the application callback so an ordinary C flag-posting function is usable.
 push ix
 push iy
 exx
 push bc
 push de
 push hl
 exx
 ex af,af'
 push af
 ex af,af'
 xor a
 call async_callback_jump
 ex af,af'
 pop af
 ex af,af'
 exx
 pop hl
 pop de
 pop bc
 exx
 pop iy
 pop ix
 ret
async_callback_jump:
 jp (hl)
async_cts_blocked:
 ld a,(async_saved_ier)
 or 8                      ; Mask level-triggered TX-empty while CTS is blocked.
 out0 (UART0_IER),a
 in0 a,(UART0_MSR)          ; Do not lose a rising CTS edge during the mask.
 bit 4,a
 ret z
 ld a,(async_saved_ier)
 or 0x0a
 out0 (UART0_IER),a
 ret                       ; Re-arm next entry, not a DI retry loop.

.bss
async_source: .ds 3
async_left: .ds 3
async_callback: .ds 3
async_saved_ier: .ds 1
async_enabled: .ds 1
