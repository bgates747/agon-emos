; PORT-008 STREAM-TIMING-R01: private single-sample UART drain observation.
; Called only by a fixture bound to this exact ELF, with normal EMOS ownership.
; No writes of payload/GPIO here. Reading LSR acknowledges RX error flags:
; stop the owned UART on error, as the maintained ordinary sender does.
; Return C uint24 in HL: 0 pending, 1 shift register empty, 2 error, 3 unavailable.
; One short masked sample; no interrupt-disabled wait or timer control.
            .ASSUME ADL = 1
            DEFINE .STARTUP, SPACE = ROM
            SEGMENT .STARTUP
            XDEF _emos_stream_timing_uart
            XREF _emosVduBackend
            XREF _uart1_keyboard_owned
            XREF _emos_key_faulted
            XREF _uart1_keyboard_stop
_emos_stream_timing_uart:
            LD HL, 3
            LD A, I
            RET PO
            DI
            LD A, (_emosVduBackend)
            DEC A
            JR NZ, timing_uart_return
            LD A, (_uart1_keyboard_owned)
            DEC A
            JR NZ, timing_uart_return
            LD A, (_emos_key_faulted)
            OR A, A
            JR NZ, timing_uart_return
            IN0 A, (0D5h)
            LD E, A
            AND 09Eh
            JR NZ, timing_uart_error
            LD HL, 0
            BIT 6, E
            JR Z, timing_uart_return
            INC L
timing_uart_return:
            EI
            RET
timing_uart_error:
            CALL _uart1_keyboard_stop
            LD HL, 2
            JR timing_uart_return
            END
