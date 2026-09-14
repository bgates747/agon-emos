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
            XREF _uart1_keyboard_irq
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
