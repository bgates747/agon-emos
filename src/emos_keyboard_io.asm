;
; Resident keyboard C/interrupt bridges. No callback is run from foreground.
; C arguments use three-byte stack slots; BYTE results return in A (not HL).
;
            INCLUDE "equs.inc"
            .ASSUME ADL = 1
            DEFINE .STARTUP, SPACE = ROM
            SEGMENT .STARTUP
            XDEF _emos_keyboard_lock
            XDEF _emos_keyboard_unlock
            XDEF _emos_keyboard_vector
            XDEF _emos_keyboard_clock
            XDEF _emos_keyboard_effect
            XDEF _emos_keyboard_settings
            XDEF _emos_keyboard_mainboard_layout
            XDEF _emos_keyboard_irq_entry
            XDEF emos_keyboard_tick_entry
            XREF __2nd_jump_table
            XREF __default_mi_handler
            XREF _clock
            XDEF _uart1_keyboard_irq
            XREF _uart1_keyboard_irq_done
            XREF _emos_keyboard_tick
            XREF emos_keyboard_payload
            XREF _keydelay
            XREF UART0_serial_PUTCH
            XDEF _uart1_keyboard_put
            XREF _uart1_keyboard_owned
            XREF _emos_key_faulted

; One atomic UART1 transmit attempt, preserving the C driver's error/CTS order.
; The original C implementation remains available to host reference tests.
; Save IFF2 in AF's parity flag; restore only the caller's interrupt state.
; No frame, wait loop or timeout here. E05's C sender owns the deadline.
_uart1_keyboard_put:
            LD A, I
            DI
            PUSH AF
            LD HL, 6
            ADD HL, SP
            LD C, (HL)
            LD A, (_uart1_keyboard_owned)
            OR A
            JR Z, keyboard_put_unavailable
            LD A, (_emos_key_faulted)
            OR A
            JR NZ, keyboard_put_unavailable
            IN0 A, (0D5h)       ; UART1_LSR, read once (acknowledges errors)
            LD B, A
            AND 09Eh           ; OE | PE | FE | BI | ERR
            JR NZ, keyboard_put_error
            IN0 A, (PC_DR)
            AND 08h            ; PC3 active-low peer CTS
            JR NZ, keyboard_put_blocked
            BIT 5, B           ; UART_LSR_THRE
            JR Z, keyboard_put_empty
            LD A, C
            OUT0 (0D0h), A      ; UART1_THR
            LD A, 1            ; UART_POLL_READY
            JR keyboard_put_done
keyboard_put_error:
            IN0 A, (PC_DR)
            OR 04h             ; PC2 stop peer; preserve other lanes
            OUT0 (PC_DR), A
            XOR A
            OUT0 (0D1h), A      ; UART1_IER = 0
            LD A, 2            ; UART_POLL_ERROR
            JR keyboard_put_done
keyboard_put_unavailable:
            LD A, 3
            JR keyboard_put_done
keyboard_put_blocked:
            LD A, 4
            JR keyboard_put_done
keyboard_put_empty:
            XOR A
keyboard_put_done:
            LD C, A
            POP AF
            LD A, C            ; does not change restored parity flag
            RET PO
            EI
            RET

            XDEF _emos_keyboard_transmit_block
; TX01: enabled IRQs, private successful Deadline, 3-byte C argument slots.
; IY source, BC low16 count, IX deadline, HL 24-bit poll budget,
; D retained byte, E previous clock. IRQ entry preserves all these registers.
_emos_keyboard_transmit_block:
            PUSH IX
            PUSH IY
            LD IX, 0
            ADD IX, SP
            LD IY, (IX+9)
            LD BC, (IX+12)
            LD IX, (IX+15)
            LD HL, (IX+3)
            LD E, (IX+0)
            JR keyboard_block_count
keyboard_block_byte:
            LD D, (IY+0)
            INC IY
keyboard_block_retry:
            LD A, (_emos_key_faulted)
            OR A
            JP NZ, keyboard_block_fail
            LD A, (_clock)
            SUB E
            JR NZ, keyboard_block_tick
            DEC HL
            LD A, L
            OR H
            JR NZ, keyboard_block_attempt
            LD (IX+3), HL       ; low16 zero: examine the 24-bit high byte
            LD A, (IX+5)
            OR A
            JP Z, keyboard_block_fail
keyboard_block_attempt:
            DI
            LD A, (_uart1_keyboard_owned)
            OR A
            JR Z, keyboard_block_unavailable
            LD A, (_emos_key_faulted)
            OR A
            JR NZ, keyboard_block_unavailable
            IN0 A, (0D5h)
            AND 0BEh           ; TX03: errors or THRE, one acknowledging sample
            CP 020h
            JR NZ, keyboard_block_status
            IN0 A, (PC_DR)
            AND 08h
            JR NZ, keyboard_block_wait
            LD A, D
            OUT0 (0D0h), A
            EI
            DEC BC
keyboard_block_count:
            LD A, B
            OR C
            JR NZ, keyboard_block_byte
            LD A, 1
            JR keyboard_block_return
keyboard_block_status:
            OR A
            JR NZ, keyboard_block_error
            IN0 A, (PC_DR)     ; clean-empty still samples CTS, exactly as C
keyboard_block_wait:
            EI
            JR keyboard_block_retry
keyboard_block_unavailable:
            EI
keyboard_block_fail:
            XOR A
keyboard_block_return:
            LD (IX+3), HL
            POP IY
            POP IX
            RET
keyboard_block_error:
            IN0 A, (PC_DR)
            OR 04h
            OUT0 (PC_DR), A
            XOR A
            OUT0 (0D1h), A
            EI
            LD A, 2
            JR keyboard_block_return
keyboard_block_tick:
            PUSH BC
            LD BC, 0
            LD C, A            ; modulo-8-bit clock delta
            LD HL, 0
            LD L, (IX+1)
            LD H, (IX+2)
            ADD HL, BC
            LD (IX+1), L       ; elapsed uses exactly 16 bits
            LD (IX+2), H
            LD A, E
            ADD A, C
            LD E, A
            LD (IX+0), A
            POP BC
            LD HL, 262144
            LD A, (IX+2)
            CP 2
            JP C, keyboard_block_attempt
            JP NZ, keyboard_block_fail
            LD A, (IX+1)
            CP 88              ; 600 = 0x258
            JP C, keyboard_block_attempt
            JP keyboard_block_fail

            XDEF _emos_keyboard_byte
            XREF _emos_key_rx
            XREF _emos_keyboard_dispatch
            XREF _emos_keyboard_fault
; RX01: private layout checked in C: state/cmd/len/remaining/used/age/payload.
; IRQ entry already protects interrupted primary and alternate registers.
; Existing C dispatch and stock effect bridges still own complete packets.
_emos_keyboard_byte:
            LD A, (_emos_key_faulted)
            OR A
            RET NZ
            LD A, (_uart1_keyboard_owned)
            OR A
            RET Z
            LD HL, 3
            ADD HL, SP
            LD C, (HL)
            JR keyboard_rx_state
; RX04/RX05: private same-file entry, called only by the masked FIFO drain.
; That caller checks fault before its first byte and after every parser return.
; No interrupt or intervening writer can invalidate fault-clear here. Retain
; owner admission: a dispatch may release it without faulting. Public C above
; retains both checks. The complete-IRQ harness enforces this private contract.
keyboard_rx_register:
            LD A, (_uart1_keyboard_owned)
            OR A
            RET Z
keyboard_rx_state:
            LD A, (_emos_key_rx)
            OR A
            JR Z, keyboard_rx_header
            DEC A
            JR Z, keyboard_rx_length
            LD A, (_emos_key_rx+4)
            CP 240
            JR NC, keyboard_rx_discard
            LD DE, 0
            LD E, A
            LD HL, _emos_key_rx+6
            ADD HL, DE
            LD (HL), C
            INC A
            LD (_emos_key_rx+4), A
keyboard_rx_discard:
            LD A, (_emos_key_rx+3)
            DEC A
            LD (_emos_key_rx+3), A
            RET NZ
            LD (_emos_key_rx), A
            LD A, (_emos_key_rx+2)
            CP 241
            RET NC
            JP _emos_keyboard_dispatch
keyboard_rx_header:
            BIT 7, C
            RET Z
            LD A, C
            LD (_emos_key_rx+1), A
            LD A, 1
            LD (_emos_key_rx), A
            LD A, (_clock)
            LD (_emos_key_rx+5), A
            RET
keyboard_rx_length:
            LD A, C
            LD (_emos_key_rx+2), A
            LD (_emos_key_rx+3), A
            XOR A
            LD (_emos_key_rx+4), A
            LD A, (_emos_key_rx+1)
            CP 081h
            JR NZ, keyboard_rx_settings_length
            LD A, C
            CP 4
            JP NZ, _emos_keyboard_fault
keyboard_rx_settings_length:
            LD A, (_emos_key_rx+1)
            CP 088h
            JR NZ, keyboard_rx_body_start
            LD A, C
            CP 5
            JP NZ, _emos_keyboard_fault
keyboard_rx_body_start:
            LD A, C
            OR A
            JR Z, keyboard_rx_empty
            LD A, 2
keyboard_rx_empty:
            LD (_emos_key_rx), A
            RET

; RX02: same bounded FIFO drain under the unchanged full IRQ register save.
; RX04 keeps BC saved across callbacks and passes its existing C byte directly.
_uart1_keyboard_irq:
            LD A, (_uart1_keyboard_owned)
            OR A
            RET Z
            LD A, (_emos_key_faulted)
            OR A
            RET NZ
            IN0 A, (PC_DR)
            OR 04h
            OUT0 (PC_DR), A
            LD B, 16
keyboard_irq_drain:
            IN0 C, (0D5h)
            LD A, C
            AND 09Eh
            JP NZ, _emos_keyboard_fault
            BIT 0, C
            JR Z, keyboard_irq_complete
            IN0 C, (0D0h)
            PUSH BC
            CALL keyboard_rx_register
            POP BC
            LD A, (_emos_key_faulted)
            OR A
            RET NZ
            DJNZ keyboard_irq_drain
keyboard_irq_complete:
            JP _uart1_keyboard_irq_done

_emos_keyboard_lock:
            LD A, I
            DI
            JP PO, keyboard_lock_disabled
            LD A, 1
            RET
keyboard_lock_disabled:
            XOR A
            RET
_emos_keyboard_unlock:
            LD HL, 3
            ADD HL, SP
            LD A, (HL)
            OR A
            RET Z
            EI
            RET
_emos_keyboard_clock:
            LD A, (_clock)
            RET

; Caller masks interrupts. 1 = claim if default; 0 = release if ours;
; 2 = check still ours. Never restore a saved vector over another owner.
_emos_keyboard_vector:
            LD HL, 3
            ADD HL, SP
            LD A, (HL)
            LD B, A
            LD HL, (__2nd_jump_table + 035h)
            LD DE, _emos_keyboard_irq_entry
            CP 1
            JR NZ, keyboard_vector_compare
            LD DE, __default_mi_handler
keyboard_vector_compare:
            OR A
            SBC HL, DE
            JR NZ, keyboard_vector_fail
            LD A, B
            CP 2
            JR Z, keyboard_vector_success
            LD DE, __default_mi_handler
            OR A
            JR Z, keyboard_vector_store
            LD DE, _emos_keyboard_irq_entry
keyboard_vector_store:
            LD (__2nd_jump_table + 035h), DE
keyboard_vector_success:
            LD A, 1
            RET
keyboard_vector_fail:
            XOR A
            RET

_emos_keyboard_effect:
            ; Stock callbacks are assembly entry points, not C callees.
            ; Protect the interrupted C frame even if a callback uses IX/IY.
            PUSH IX
            PUSH IY
            LD HL, 9
            ADD HL, SP
            LD DE, (HL)
            CALL emos_keyboard_payload
            POP IY
            POP IX
            RET
_emos_keyboard_settings:
            LD HL, 3
            ADD HL, SP
            LD HL, (HL)
            LD DE, _keydelay
            LD BC, 5
            LDIR
            RET
_emos_keyboard_mainboard_layout:
            LD HL, 3
            ADD HL, SP
            LD A, 23
            CALL UART0_serial_PUTCH
            XOR A
            CALL UART0_serial_PUTCH
            LD A, 081h
            CALL UART0_serial_PUTCH
            LD A, (HL)
            JP UART0_serial_PUTCH

; Save both primary and alternate registers around C, including IX/IY.
; A tick can interrupt C while it is using alternate registers in a helper.
_emos_keyboard_irq_entry:
            DI
            PUSH AF
            PUSH BC
            PUSH DE
            PUSH HL
            PUSH IX
            PUSH IY
            EX AF, AF'
            PUSH AF
            EXX
            PUSH BC
            PUSH DE
            PUSH HL
            CALL _uart1_keyboard_irq
            POP HL
            POP DE
            POP BC
            EXX
            POP AF
            EX AF, AF'
            POP IY
            POP IX
            POP HL
            POP DE
            POP BC
            POP AF
            EI
            RETI.L

emos_keyboard_tick_entry:
            PUSH IX
            PUSH IY
            EX AF, AF'
            PUSH AF
            EXX
            PUSH BC
            PUSH DE
            PUSH HL
            CALL _emos_keyboard_tick
            POP HL
            POP DE
            POP BC
            EXX
            POP AF
            EX AF, AF'
            POP IY
            POP IX
            RET
