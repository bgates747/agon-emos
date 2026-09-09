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
