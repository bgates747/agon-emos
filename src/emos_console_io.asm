; EMOS-001: maintained native import from EMOS6ececd56.
; Initial dialect conversion by mos-agondev; review/edit this source directly.
; Private foreground/IRQ bridges. BYTE arguments use 3-byte ADL stack slots.
            .ASSUME ADL = 1
            ; zds2gas: DEFINE .STARTUP, SPACE = ROM
            .text
            XDEF _emos_mainboard_put
            XDEF _emos_vdp_effect
            XREF UART0_serial_PUTCH
            XREF emos_vdp_dispatch
_emos_mainboard_put:
            LD HL, 3
            ADD HL, SP
            LD A, (HL)
            JP UART0_serial_PUTCH
_emos_vdp_effect:
            LD HL, 3
            ADD HL, SP
            LD A, (HL)
            SUB 0x080
            JP emos_vdp_dispatch

; INTEG-019: C-to-existing MOS callback bridge. The UART1 ISR owns payload
; lifetime; DE borrows its private record until callback return. No key effect.
; Preserve C callee-saved IX/IY even for an older application callback.
            XDEF _emos_reply_callback
            XREF _user_kbvector
_emos_reply_callback:
            PUSH IX
            LD IX, 0
            ADD IX, SP
            PUSH IY
            LD DE, (IX+6)
            LD HL, (_user_kbvector)
            LD BC, 0
            OR A
            SBC HL, BC
            JR Z, net_callback_done
            LD BC, net_callback_done
            PUSH BC
            JP (HL)
net_callback_done:
            POP IY
            POP IX
            RET
