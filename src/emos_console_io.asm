; Private foreground/IRQ bridges. BYTE arguments use 3-byte ADL stack slots.
            .ASSUME ADL = 1
            DEFINE .STARTUP, SPACE = ROM
            SEGMENT .STARTUP
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
            SUB 080h
            JP emos_vdp_dispatch
