; EMOS-001: maintained native import from EMOS6ececd56.
; Initial dialect conversion by mos-agondev; review/edit this source directly.
; Private slice leaf. Entry: admitted UART parked, VALID low, CLOCK high,
; Port C input, length1..512. Exit keeps those levels for utility FatFS I/O.
; No admission/ack/tail per slice. Preserve registers and entry IFF2; only the
; edge-pair RMW masks interrupts. Same optimized RX cost as native byte loop.
            .include "equs.inc"
            .ASSUME ADL = 1
            ; zds2gas: DEFINE .STARTUP, SPACE = ROM
            .text
            XDEF _emos_parallel_upload_bytes
_emos_parallel_upload_bytes:
            PUSH IY
            LD IY, 0
            ADD IY, SP
            PUSH BC
            PUSH DE
            PUSH HL
            LD HL, (IY+6)
            LD DE, (IY+9)
            LD C, 0x010
            LD B, E
            LD A, E
            OR A, A
            JR Z, upload_receive_pages
            INC D
upload_receive_pages:
            LD A, I
            JP PO, upload_receive_masked
upload_receive:
            IN0 A, (PD_DR)
            BIT 4, A
            JP NZ, upload_receive_fault
            DI
            IN0 A, (PD_DR)
            AND 0x04F
            OR A, C
            OUT0 (PD_DR), A
            OR A, 0x020
            OUT0 (PD_DR), A
            EI
            IN0 A, (PC_DR)
            LD (HL), A
            INC HL
            DJNZ upload_receive
            DEC D
            JR NZ, upload_receive
            JR upload_receive_ok
upload_receive_masked:
            IN0 A, (PD_DR)
            BIT 4, A
            JP NZ, upload_receive_fault
            IN0 A, (PD_DR)
            AND 0x04F
            OR A, C
            OUT0 (PD_DR), A
            OR A, 0x020
            OUT0 (PD_DR), A
            IN0 A, (PC_DR)
            LD (HL), A
            INC HL
            DJNZ upload_receive_masked
            DEC D
            JR NZ, upload_receive_masked
upload_receive_ok:
            XOR A, A
            JR upload_receive_return
upload_receive_fault:
            LD A, 0x0E5
upload_receive_return:
            POP HL
            POP DE
            POP BC
            LD SP, IY
            POP IY
            RET
            ; zds2gas: END
