;
; Title:	AGON MOS - UART code
; Author:	Dean Belfield
; Created:	11/07/2022
; Last Updated:	29/03/2023
;
; Modinfo:
; 27/07/2022:	Reverted serial_TX back to use RET, not RET.L and increased timeout
; 08/08/2022:	Added check_CTS
; 22/03/2023:	Added serial_PUTCH, moved putch and getch from uart.c
; 23/03/2023:	Renamed serial_RX_WAIT to seral_GETCH
; 29/03/2023:	Added support for UART1

			INCLUDE	"macros.inc"
			INCLUDE	"equs.inc"

			.ASSUME	ADL = 1
			
			DEFINE .STARTUP, SPACE = ROM
			SEGMENT .STARTUP
			
			XDEF	UART0_serial_TX
			XDEF	UART0_serial_RX
			XDEF	UART0_serial_GETCH
			XDEF	UART0_serial_PUTCH 

			XDEF	UART1_serial_TX
			XDEF	UART1_serial_RX
			XDEF	UART1_serial_GETCH
			XDEF	UART1_serial_PUTCH
			XDEF	EMOS_vdu_PUTCH
			XDEF	EMOS_vdu_WRITE
			XDEF	_emos_port008_prepare
			XDEF	_emos_port008_recover

			XDEF	_putch
			XDEF	_getch 
			
			XDEF	putch 		
			XDEF	getch 

			XREF	_serialFlags	; In globals.asm
			XREF	_emosVduBackend	; Core-owned committed VDU route
				
UART0_PORT		EQU	%C0		; UART0
UART1_PORT		EQU	%D0		; UART1
				
UART0_REG_RBR:		EQU	UART0_PORT+0	; Receive buffer
UART0_REG_THR:		EQU	UART0_PORT+0	; Transmitter holding
UART0_REG_DLL:		EQU	UART0_PORT+0	; Divisor latch low
UART0_REG_IER:		EQU	UART0_PORT+1	; Interrupt enable
UART0_REG_DLH:		EQU	UART0_PORT+1	; Divisor latch high
UART0_REG_IIR:		EQU	UART0_PORT+2	; Interrupt identification
UART0_REG_FCT:		EQU	UART0_PORT+2;	; Flow control
UART0_REG_LCR:		EQU	UART0_PORT+3	; Line control
UART0_REG_MCR:		EQU	UART0_PORT+4	; Modem control
UART0_REG_LSR:		EQU	UART0_PORT+5	; Line status
UART0_REG_MSR:		EQU	UART0_PORT+6	; Modem status
UART0_REG_SCR:		EQU 	UART0_PORT+7	; Scratch

UART1_REG_RBR:		EQU	UART1_PORT+0	; Receive buffer
UART1_REG_THR:		EQU	UART1_PORT+0	; Transmitter holding
UART1_REG_DLL:		EQU	UART1_PORT+0	; Divisor latch low
UART1_REG_IER:		EQU	UART1_PORT+1	; Interrupt enable
UART1_REG_DLH:		EQU	UART1_PORT+1	; Divisor latch high
UART1_REG_IIR:		EQU	UART1_PORT+2	; Interrupt identification
UART1_REG_FCT:		EQU	UART1_PORT+2;	; Flow control
UART1_REG_LCR:		EQU	UART1_PORT+3	; Line control
UART1_REG_MCR:		EQU	UART1_PORT+4	; Modem control
UART1_REG_LSR:		EQU	UART1_PORT+5	; Line status
UART1_REG_MSR:		EQU	UART1_PORT+6	; Modem status
UART1_REG_SCR:		EQU 	UART1_PORT+7	; Scratch

TX_WAIT			EQU	16384 		; Count before a TX times out

; Fixed-purpose PORT-008 forward-only prototype. These r01 GPIO constants and
; routines are intentionally resident beside the semantic VDU dispatcher so a
; committed backend cannot bypass EMOS ownership. They remain unreachable in
; ordinary builds because only the explicit PORT-008 source profile can select
; backend 2. This is predecessor wiring, not the V1 r02 electrical contract.
PORT008_READY_BIT	EQU	10h		; PD4, active low input
PORT008_CLOCK_BIT	EQU	20h		; PD5, sender output
PORT008_VALID_BIT	EQU	80h		; PD7, active low output
PORT008_KEEP_CTRL	EQU	4Fh		; Clear PD4, PD5, PD7 only
PORT008_KEEP_CLOCK	EQU	DFh
PORT008_KEEP_VALID	EQU	7Fh
PORT008_OUTPUT_DDR	EQU	5Fh		; PD4 input; PD5/PD7 output
PORT008_WAIT_READY	EQU	160
PORT008_WAIT_RELEASE	EQU	32
EMOS_VDU_EDP_EXTENDED	EQU	2
EMOS_UNAVAILABLE_STATUS EQU	35

UART_LSR_ERR		EQU 	%80		; Error
UART_LSR_ETX		EQU 	%40		; Transmit empty
UART_LSR_ETH		EQU	%20		; Transmit holding register empty
UART_LSR_RDY		EQU	%01		; Data ready

; Check whether we're clear to send (UART0 only)
;
UART0_wait_CTS:		IN0		A, (UART0_REG_MSR)
			BIT		4, A			; check inverted CTS bit, 1 = CTS, 0 = NOT CTS (clear to send)
			JR		Z, UART0_wait_CTS
			RET

UART1_wait_CTS:		GET_GPIO	PC_DR, 8		; Check Port C, bit 3 (CTS)
			JR		NZ, UART1_wait_CTS
			RET

; Write a character to UART0
; Parameters:
; - A: Data to write
; Returns:
; - F: C if written
; - F: NC if timed out
;
UART0_serial_TX:	PUSH		BC			; Stack BC
			PUSH		AF 			; Stack AF
			LD		BC,TX_WAIT		; Set CB to the transmit timeout
UART0_serial_TX1:	IN0		A,(UART0_REG_LSR)	; Get the line status register
			AND 		UART_LSR_ETH		; Check for TX hold register empty
			JR		NZ, UART0_serial_TX2	; If set, then TX is empty, goto transmit
			DEC		BC
			LD		A, B
			OR		C
			JR		NZ, UART0_serial_TX1
			POP		AF			; We've timed out at this point so
			POP		BC			; Restore the stack
			OR		A			; Clear the carry flag and preserve A
			RET	
UART0_serial_TX2:	POP		AF			; Good to send at this point, so
			OUT0		(UART0_REG_THR),A	; Write the character to the UART transmit buffer
			POP		BC			; Restore BC
			SCF					; Set the carry flag
			RET 

; Write a character to UART1
; Parameters:
; - A: Data to write
; Returns:
; - F: C if written
; - F: NC if timed out
;
UART1_serial_TX:	PUSH		BC			; Stack BC
			PUSH		AF 			; Stack AF
			LD		BC,TX_WAIT		; Set CB to the transmit timeout
UART1_serial_TX1:	IN0		A,(UART1_REG_LSR)	; Get the line status register
			AND 		UART_LSR_ETH		; Check for TX hold register empty
			JR		NZ, UART1_serial_TX2	; If set, then TX is empty, goto transmit
			DEC		BC
			LD		A, B
			OR		C
			JR		NZ, UART1_serial_TX1
			POP		AF			; We've timed out at this point so
			POP		BC			; Restore the stack
			OR		A			; Clear the carry flag and preserve A
			RET	
UART1_serial_TX2:	POP		AF			; Good to send at this point, so
			OUT0		(UART1_REG_THR),A	; Write the character to the UART transmit buffer
			POP		BC			; Restore BC
			SCF					; Set the carry flag
			RET 

; Read a character from UART0
; Returns:
; - A: Data read
; - F: C if character read
; - F: NC if no character read
;
UART0_serial_RX:	IN0		A,(UART0_REG_LSR)	; Get the line status register
			AND 		UART_LSR_RDY		; Check for characters in buffer
			RET		Z			; Just ret (with carry clear) if no characters
			IN0		A,(UART0_REG_RBR)	; Read the character from the UART receive buffer
			SCF 					; Set the carry flag
			RET

; Read a character from UART1
; Returns:
; - A: Data read
; - F: C if character read
; - F: NC if no character read
;
UART1_serial_RX:	IN0		A,(UART1_REG_LSR)	; Get the line status register
			AND 		UART_LSR_RDY		; Check for characters in buffer
			RET		Z			; Just ret (with carry clear) if no characters
			IN0		A,(UART1_REG_RBR)	; Read the character from the UART receive buffer
			SCF 					; Set the carry flag
			RET

; Read a character from UART0 (blocking)
; Returns:
; - A: Data read
; - F: C if read
; - F: NC if UART not enabled
;
UART0_serial_GETCH:	PUSH		AF 
			LD		A, (_serialFlags)
			TST		01h
			JR		Z, UART_serial_NE
			POP		AF
$$:			CALL 		UART0_serial_RX
			JR		NC,$B
			RET 

; Read a character from UART1 (blocking)
; Returns:
; - A: Data read
; - F: C if read
; - F: NC if UART not enabled
;
UART1_serial_GETCH:	PUSH		AF 
			LD		A, (_serialFlags)
			TST		10h
			JR		Z, UART_serial_NE
			POP		AF
$$:			CALL 		UART1_serial_RX
			JR		NC,$B
			RET 

; Write a character to UART0 (blocking)
; Parameters:
; - A: Character to write out
; Returns:
; - F: C if written
; - F: NC if UART not enabled
;
UART0_serial_PUTCH:	PUSH	AF
			LD	A, (_serialFlags)		; Get the serial flags
			TST	01h				; Check UART is enabled
			JR	Z, UART_serial_NE		; If not, then skip
			TST	02h				; If hardware flow control enabled then
			CALL	NZ, UART0_wait_CTS		; Wait for clear to send signal
			POP	AF
$$:			CALL	UART0_serial_TX			; Send the character
			JR	NC, $B				; Repeat until sent
			RET

; Write a character to UART1 (blocking)
; Parameters:
; - A: Character to write out
; Returns:
; - F: C if written
; - F: NC if UART not enabled
;
UART1_serial_PUTCH:	PUSH	AF
			LD	A, (_serialFlags)		; Get the serial flags
			TST	10h				; Check UART is enabled
			JR	Z, UART_serial_NE		; If not, then skip (reuses UART0 routine here)
			TST	20h				; If hardware flow control enabled then
			CALL	NZ, UART1_wait_CTS		; Wait for clear to send signal
			POP	AF			
$$:			CALL	UART1_serial_TX			; Send the character
			JR	NC, $B				; Repeat until sent
			RET

; Core EMOS semantic VDU dispatcher. Snapshot the committed backend once for
; this character. Backend 0 is the stock onboard VDP. Backend 2 is reachable
; only from the fixed-purpose PORT-008 build after its Exclusive Extended
; prepare transaction. Every other value fails closed.
;
EMOS_vdu_PUTCH:	PUSH	AF
			LD	A, (_emosVduBackend)
			OR	A, A
			JR	Z, EMOS_vdu_onboard
			CP	EMOS_VDU_EDP_EXTENDED
			JR	Z, EMOS_vdu_port008
			POP	AF
			OR	A, A				; Unsupported backend: clear carry
			RET
EMOS_vdu_port008:
			POP	AF
			JP	PORT008_vdu_PUTCH
EMOS_vdu_onboard:
			POP	AF
			JP	UART0_serial_PUTCH

; Called by UART0 and UART1 PUTCH and GETCH if the UART is not enabled. Keep
; this stock helper adjacent to the semantic dispatcher so the original short
; branches and raw UART routines remain byte-for-byte unchanged.
;
UART_serial_NE:		POP	AF				; Tidy up the stack
			OR	A 				; Clear the carry flag
			RET

; Write one ordinary bounded VDU block. The onboard route deliberately keeps
; the stock byte-at-a-time UART behavior. PORT-008 maps the entire block to one
; physical record while preserving the stock RST 18h postcondition (HL advanced
; by the original count and BC zero). No physical-record metadata enters the
; VDU byte stream.
;
EMOS_vdu_WRITE:	LD	A, (_emosVduBackend)
			OR	A, A
			JR	Z, EMOS_vdu_WRITE_onboard
			CP	EMOS_VDU_EDP_EXTENDED
			JR	Z, EMOS_vdu_WRITE_port008
			OR	A, A				; Unsupported backend: clear carry
			RET
EMOS_vdu_WRITE_onboard:
			LD	A, (HL)
			CALL	UART0_serial_PUTCH
			INC	HL
			DEC	BC
			LD	A, B
			OR	C
			JR	NZ, EMOS_vdu_WRITE_onboard
			RET
EMOS_vdu_WRITE_port008:
			PUSH	BC
			PUSH	HL
			CALL	PORT008_send
			POP	HL
			POP	BC
			ADD	HL, BC
			LD	BC, 0
			OR	A, A
			RET	NZ
			SCF
			RET

; Send one ordinary VDU byte as one physical r01 record. Record boundaries are
; below the retained VDU byte stream and carry no application semantics.
;
PORT008_vdu_PUTCH:	PUSH	AF
			LD	(_port008_byte), A
			LD	HL, _port008_byte
			LD	BC, 1
			CALL	PORT008_send
			OR	A, A
			JR	NZ, PORT008_vdu_PUTCH_failed
			POP	AF
			SCF
			RET
PORT008_vdu_PUTCH_failed:
			POP	AF
			OR	A, A				; Preserve byte, report not written
			RET

; Acquire the predecessor GPIO mapping and send the stock nonzero General Poll
; request that releases VDP v2.16.0's existing startup wait. The return packet
; is intentionally discarded by the P4 Stream and cannot prove EDP identity.
; C ABI: int emos_port008_prepare(void), result in HLU.
;
_emos_port008_prepare:
			LD	A, (_port008_acquired)
			OR	A, A
			JP	NZ, PORT008_prepare_poll
			IN0	A, (PC_DR)
			LD	(_port008_saved_pc_dr), A
			IN0	A, (PC_DDR)
			LD	(_port008_saved_pc_ddr), A
			IN0	A, (PC_ALT1)
			LD	(_port008_saved_pc_alt1), A
			IN0	A, (PC_ALT2)
			LD	(_port008_saved_pc_alt2), A
			IN0	A, (PD_DR)
			LD	(_port008_saved_pd_dr), A
			IN0	A, (PD_DDR)
			LD	(_port008_saved_pd_ddr), A
			IN0	A, (PD_ALT1)
			LD	(_port008_saved_pd_alt1), A
			IN0	A, (PD_ALT2)
			LD	(_port008_saved_pd_alt2), A

			; Release every affected pin before changing its GPIO function.
			LD	A, 0FFh
			OUT0	(PC_DDR), A
			IN0	A, (PD_DDR)
			OR	PORT008_READY_BIT | PORT008_CLOCK_BIT | PORT008_VALID_BIT
			OUT0	(PD_DDR), A
			XOR	A, A
			OUT0	(PC_ALT1), A
			OUT0	(PC_ALT2), A
			OUT0	(PC_DR), A
			IN0	A, (PD_ALT1)
			AND	PORT008_KEEP_CTRL
			OUT0	(PD_ALT1), A
			IN0	A, (PD_ALT2)
			AND	PORT008_KEEP_CTRL
			OUT0	(PD_ALT2), A
			IN0	A, (PD_DR)
			AND	PORT008_KEEP_CLOCK
			OR	PORT008_CLOCK_BIT | PORT008_VALID_BIT
			LD	(_port008_idle_high), A
			AND	PORT008_KEEP_CLOCK
			LD	(_port008_idle_low), A
			LD	A, (_port008_idle_high)
			AND	PORT008_KEEP_VALID
			LD	(_port008_active_high), A
			AND	PORT008_KEEP_CLOCK
			LD	(_port008_active_low), A
			LD	A, (_port008_idle_high)
			OUT0	(PD_DR), A
			LD	A, 0
			OUT0	(PC_DDR), A
			IN0	A, (PD_DDR)
			OR	PORT008_READY_BIT
			AND	PORT008_OUTPUT_DDR
			OUT0	(PD_DDR), A
			LD	A, 1
			LD	(_port008_acquired), A
PORT008_prepare_poll:
			LD	HL, _port008_general_poll
			LD	BC, 4
			CALL	PORT008_send
			OR	A, A
			JR	Z, PORT008_prepare_return
			PUSH	AF
			CALL	_emos_port008_recover
			POP	AF
PORT008_prepare_return:
			LD	HL, 0
			LD	L, A
			RET

; Restore the complete Port C/Port D register snapshots. Inputs are selected
; before latches and alternate functions are restored, preventing a recovery
; transition from driving an unintended saved value onto r01.
; C ABI: void emos_port008_recover(void).
;
_emos_port008_recover:
			LD	A, (_port008_acquired)
			OR	A, A
			RET	Z
			LD	A, 0FFh
			OUT0	(PC_DDR), A
			IN0	A, (PD_DDR)
			OR	PORT008_READY_BIT | PORT008_CLOCK_BIT | PORT008_VALID_BIT
			OUT0	(PD_DDR), A
			LD	A, (_port008_saved_pc_dr)
			OUT0	(PC_DR), A
			LD	A, (_port008_saved_pc_alt1)
			OUT0	(PC_ALT1), A
			LD	A, (_port008_saved_pc_alt2)
			OUT0	(PC_ALT2), A
			LD	A, (_port008_saved_pc_ddr)
			OUT0	(PC_DDR), A
			LD	A, (_port008_saved_pd_dr)
			OUT0	(PD_DR), A
			LD	A, (_port008_saved_pd_alt1)
			OUT0	(PD_ALT1), A
			LD	A, (_port008_saved_pd_alt2)
			OUT0	(PD_ALT2), A
			LD	A, (_port008_saved_pd_ddr)
			OUT0	(PD_DDR), A
			XOR	A, A
			LD	(_port008_acquired), A
			LD	(_port008_busy), A
			RET

; Send one nonempty variable-length record. HLU points to bytes and BC is the
; 16-bit length. This private prototype caller currently emits only one- and
; four-byte records; the P4 endpoint independently rejects records above 4096.
; Result A is zero, EMOS_UNAVAILABLE, or 1/2 for READY/completion timeout.
;
PORT008_send:		LD	A, (_port008_acquired)
			OR	A, A
			JP	Z, PORT008_unavailable
			LD	A, B
			OR	C
			JP	Z, PORT008_unavailable
			LD	A, B				; P4 accepts at most 0x1000 bytes
			CP	10h
			JR	C, PORT008_size_valid
			JP	NZ, PORT008_unavailable
			LD	A, C
			OR	A, A
			JP	NZ, PORT008_unavailable
PORT008_size_valid:
			LD	A, (_port008_busy)
			OR	A, A
			JP	NZ, PORT008_unavailable
			LD	(_port008_pointer), HL
			LD	(_port008_length), BC
			LD	A, I				; Preserve IEF2 like predecessor
			JP	PO, PORT008_iff_was_off
			LD	A, 1
			JR	PORT008_save_iff
PORT008_iff_was_off:
			XOR	A, A
PORT008_save_iff:
			LD	(_port008_saved_iff), A
			DI
			LD	A, 1
			LD	(_port008_busy), A
			LD	A, PORT008_WAIT_READY
			LD	(_port008_wait_windows), A
			LD	BC, 0
PORT008_wait_ready:
			LD	A, (_port008_idle_high)
			OUT0	(PD_DR), A
			LD	A, (_port008_idle_low)
			OUT0	(PD_DR), A
			IN0	A, (PD_DR)
			AND	PORT008_READY_BIT
			JR	Z, PORT008_admitted
			DEC	BC
			LD	A, B
			OR	C
			JR	NZ, PORT008_wait_ready
			LD	A, (_port008_wait_windows)
			DEC	A
			LD	(_port008_wait_windows), A
			LD	BC, 0
			JR	NZ, PORT008_wait_ready
			LD	A, 1
			JR	PORT008_finish

PORT008_admitted:
			LD	HL, (_port008_pointer)
			LD	BC, (_port008_length)
PORT008_byte_loop:
			LD	A, (HL)
			OUT0	(PC_DR), A
			LD	A, (_port008_active_high)
			OUT0	(PD_DR), A
			LD	A, (_port008_active_low)
			OUT0	(PD_DR), A			; Falling-edge sample
			INC	HL
			DEC	BC
			LD	A, B
			OR	C
			JR	NZ, PORT008_byte_loop
			LD	A, (_port008_idle_low)		; VALID inactive
			OUT0	(PD_DR), A
			LD	A, PORT008_WAIT_RELEASE
			LD	(_port008_wait_windows), A
			LD	BC, 0
PORT008_wait_release:
			LD	A, (_port008_idle_high)
			OUT0	(PD_DR), A
			LD	A, (_port008_idle_low)
			OUT0	(PD_DR), A
			IN0	A, (PD_DR)
			AND	PORT008_READY_BIT
			JR	NZ, PORT008_ok
			DEC	BC
			LD	A, B
			OR	C
			JR	NZ, PORT008_wait_release
			LD	A, (_port008_wait_windows)
			DEC	A
			LD	(_port008_wait_windows), A
			LD	BC, 0
			JR	NZ, PORT008_wait_release
			LD	A, 2
			JR	PORT008_finish
PORT008_ok:		XOR	A, A
PORT008_finish:	PUSH	AF
			LD	A, (_port008_idle_high)
			OUT0	(PD_DR), A
			XOR	A, A
			LD	(_port008_busy), A
			LD	A, (_port008_saved_iff)
			OR	A, A
			JR	Z, PORT008_return
			EI
PORT008_return:
			POP	AF
			RET
PORT008_unavailable:
			LD	A, EMOS_UNAVAILABLE_STATUS
			RET

; The C wrappers
;

; INT putch(INT ch);
;
; Write a character out to the UART
; Parameters:
; - ch: The character to write (least significant byte)
; Returns:
; - The character written
;
_putch:
putch:			PUSH	IY				; Standard C prologue
			LD	IY, 0
			ADD	IY, SP	

			LD	A, (IY+6)			; INT ch (least significant byte)
			LD	HL, 0				; HLU: The return value
			LD	L, A 
			CALL	EMOS_vdu_PUTCH			; Output through fixed semantic dispatcher

			LD 	SP, IY				; Standard epilogue
			POP	IY
			RET

; INT getch(VOID);
;
; Read a character out to the UART - waits for character input
; Returns:
; - The character read
;
_getch:
getch:			PUSH	IY				; Standard C prologue
			LD	IY, 0
			ADD	IY, SP	

			CALL	UART0_serial_GETCH		; Read the character
			LD	HL, 0				; HLU: The return value
			LD	L, A

			LD 	SP, IY				; Standard epilogue
			POP	IY
			RET

			; Private prototype state is in ordinary zeroed BSS. It is not a
			; public sysvar block and no application receives these addresses.
			SECTION BSS
_port008_acquired:	DS	1
_port008_busy:		DS	1
_port008_saved_iff:	DS	1
_port008_wait_windows:	DS	1
_port008_byte:		DS	1
_port008_pointer:	DS	3
_port008_length:	DS	2
_port008_idle_low:	DS	1
_port008_idle_high:	DS	1
_port008_active_low:	DS	1
_port008_active_high:	DS	1
_port008_saved_pc_dr:	DS	1
_port008_saved_pc_ddr:	DS	1
_port008_saved_pc_alt1:	DS	1
_port008_saved_pc_alt2:	DS	1
_port008_saved_pd_dr:	DS	1
_port008_saved_pd_ddr:	DS	1
_port008_saved_pd_alt1:	DS	1
_port008_saved_pd_alt2:	DS	1

			SECTION .STARTUP
_port008_general_poll:	DB	23, 0, 80h, 1
