; PORT-008 EX02. Called only inside the fully saved VBlank bridge with the
; ExExt guard held. Admit UART/clock IRQs during bounded coordinator waits.
; Nested VBlank skips this service. Packet effects run after DI on return.
            .ASSUME ADL = 1
            DEFINE .STARTUP, SPACE = ROM
            SEGMENT .STARTUP
            XDEF _emos_exext_interrupt_service
            XREF _emos_exext_service
_emos_exext_interrupt_service:
            EI
            CALL _emos_exext_service
            DI
            RET
            END
