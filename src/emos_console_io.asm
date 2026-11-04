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
