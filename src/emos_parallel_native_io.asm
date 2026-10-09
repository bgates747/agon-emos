;
; PORT-008 F02c3 private raw bytes, called only by the guarded C binding.
; EMOS clocks BOTH directions. TX: data then falling sample, rising recovery.
; RX: falling shift, rising recovery, sample stable data. Exact length supplied
; by the admitted foreground owner, 1..4096, no indefinite waits in this leaf.
; READY loss aborts at the next byte; return releases Port C and VALID without
; restoring UART. The coordinator must still perform matched status/recovery.
; Critical sections cover ONLY Port D RMW; byte loops preserve caller IFF2.
;
            INCLUDE "equs.inc"
            .ASSUME ADL = 1
            DEFINE .STARTUP, SPACE = ROM
            SEGMENT .STARTUP
            XDEF _emos_parallel_native_bytes
_emos_parallel_native_bytes:
            PUSH IY
            LD IY, 0
            ADD IY, SP
            PUSH BC
            PUSH DE
            PUSH HL
            LD HL, (IY+6)
            LD DE, (IY+9)
            LD A, (IY+12)
            OR A, A
            JR NZ, native_receive_start
            XOR A, A
            OUT0 (PC_DDR), A
            LD C, 030h
            CALL native_control
native_send:
            IN0 A, (PD_DR)
            BIT 4, A
            JR NZ, native_fault
            LD A, (HL)
            OUT0 (PC_DR), A
            LD C, 010h
            CALL native_control
            LD C, 030h
            CALL native_control
            INC HL
            DEC DE
            LD A, D
            OR A, E
            JR NZ, native_send
            JR native_success
native_receive_start:
            LD C, 030h
            CALL native_control
native_receive:
            IN0 A, (PD_DR)
            BIT 4, A
            JR NZ, native_fault
            LD C, 010h
            CALL native_control
            LD C, 030h
            CALL native_control
            IN0 A, (PC_DR)
            LD (HL), A
            INC HL
            DEC DE
            LD A, D
            OR A, E
            JR NZ, native_receive
native_success:
            LD B, 0
            JR native_finish
native_fault:
            LD B, 0E5h
native_finish:
            LD A, 0FFh
            OUT0 (PC_DDR), A
            LD C, 0B0h
            CALL native_control
            LD A, B
            POP HL
            POP DE
            POP BC
            LD SP, IY
            POP IY
            RET
native_control:
            LD A, I
            PUSH AF
            DI
            IN0 A, (PD_DR)
            AND 04Fh
            OR A, C
            OUT0 (PD_DR), A
            POP AF
            JP PO, native_iff_off
            EI
native_iff_off:
            RET
            END
