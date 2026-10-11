;
; PORT-008 F02c3 private raw bytes, called only by the guarded C binding.
; EMOS clocks BOTH directions. TX: data then falling sample, rising recovery.
; RX: falling shift, rising recovery, sample stable data. Exact length supplied
; by the admitted foreground owner, 1..4096, no indefinite waits in this leaf.
; READY loss aborts at the next byte; return releases Port C and VALID without
; restoring UART. Sixteen inactive CLOCK pulses probe receiver EOF while
; VALID stays high and Port C stays input, ending CLOCK high for the coordinator.
; This bounded PORT-008 diagnostic tail is not a proved PARLIO timing contract.
; The coordinator must still perform matched status/recovery.
; Forward only: sixteen inactive clocks before Port C output/VALID assertion
; exercise the external-clock RX reset/alignment boundary. Reverse must not
; preclock because P4 TX has no VALID input and would consume payload bytes.
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
            ; PARLIO RX async FIFO reset crosses the external Core domain;
            ; pinned IDF's LL documents two synchronization clock cycles.
            ; PRX-06 supplied inactive clocks before admission. Probe the same
            ; boundary after READY, keeping VALID high and Port C input here.
            ; B is scratch until native_success/native_fault assigns status;
            ; DE remains the admitted length and all caller registers restore.
            LD B, 16
native_start_clocks:
            LD C, 090h
            CALL native_pulse
            DJNZ native_start_clocks
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
            CALL native_pulse
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
            CALL native_pulse
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
            ; The exact physical PRX-06 proof kept CLOCK running after VALID
            ; release and used length EOF. Native level-delimited RX instead
            ; may need inactive samples to flush EOF. Supply a bounded tail to
            ; test that boundary without adding VALID-qualified payload bytes.
            ; E is scratch here; B retains success/fault and DE is restored.
            LD E, 16
native_end_clocks:
            LD C, 090h
            CALL native_pulse
            DEC E
            JR NZ, native_end_clocks
            LD A, B
            POP HL
            POP DE
            POP BC
            LD SP, IY
            POP IY
            RET
; EX-C14: falling/rising CLOCK is one short atomic RMW, preserving the
; latest ISR-owned Port D bits. C supplies VALID/low CLOCK; the second write
; changes only CLOCK. Save/restore IFF once per pulse, never per whole block.
; Eight instructions from DI through POP AF (two more than native_control).
native_pulse:
            LD A, I
            PUSH AF
            DI
            IN0 A, (PD_DR)
            AND 04Fh
            OR A, C
            OUT0 (PD_DR), A
            OR A, 020h
            OUT0 (PD_DR), A
            POP AF
            JP PO, native_pulse_iff_off
            EI
native_pulse_iff_off:
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
