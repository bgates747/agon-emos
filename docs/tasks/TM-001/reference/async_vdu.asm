	.assume	adl = 1
	jp start
	.align 0x40
	db "MOS",0,1

UART0_THR: .equ 0xc0 ; transmit hold register
UART0_IER: .equ 0xc1 ; interrupt enable
UART0_IIR: .equ 0xc2 ; interrupt identification
UART0_LSR: .equ 0xc5

start:
	; set up custom UART0 ISR
	call async_vdu_init

	; Send data twice
	ld hl,@data_start
	ld bc,[@data_end-@data_start]
	ld de,@callback_done
	call async_vdu_send
	call @wait_done

	ld hl,@data_start
	ld bc,[@data_end-@data_start]
	ld de,@callback_done
	call async_vdu_send
	call @wait_done

	; restore stock UART0 ISR
	call async_vdu_exit

	ld hl,0
	ret

	@callback_done:
		ld a,1
		ld (@is_done),a
		ret

	@wait_done:
		xor a
		ld (@is_done),a
	    @@:
		ld a,(@is_done)
		or a
		jr z,@b
		ret

	@is_done: .db 0

	@data_start: .db "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the "
		.db "industry's standard dummy text ever since 1966, when designers at Letraset and James Mosley, the librarian at St "
		.db "Bride Printing Library in London, took a 1914 Cicero translation and scrambled it to make dummy text for "
		.db "Letraset's Body Type sheets. It has survived not only many decades, but also the leap into electronic "
		.db "typesetting, remaining essentially unchanged. It was popularised thanks to these sheets and more recently with "
		.db "desktop publishing software like Aldus PageMaker and Microsoft Word including versions of Lorem Ipsum.",13,10,13,10
	@data_end:

data_pos: .ds 3
data_count: .ds 3
txend_callback: .ds 3

async_vdu_init:
	ld hl,uart0_isr
	ld e,0x18
	ld a,0x14
	rst.lil $8
	ld (old_uart_isr+1),hl
	ret

; hl:data, bc:length, de:completion callback
async_vdu_send:
	ld (data_pos),hl
	ld (data_count),bc
	ld (txend_callback),de

	; enable FIFO-empty interrupt on uart0
	in0 a,(UART0_IER)
	or 2
	out0 (UART0_IER),a
	ret

old_uart_isr: 
	db 0xc3 ; jp nnn
	ds 3
uart0_isr:
	di
	push af
	; example interrupt identification register to see if it's
	; a fifo-empty  interrupt
	in0 a,(UART0_IIR)
	and 0xe
	cp 2
	jp nz,@f
	; is a fifo-empty interrupt
	call fill_uart0_tx_fifo
	pop af
	ei
	reti.lil
@@:
	pop af
	jp old_uart_isr

fill_uart0_tx_fifo:
	push bc
	push de
	push hl

	@waitcts:
		in0 a,(0xa2)
		tst a,8
		jr nz,@waitcts
	@not_ready3: ; wait for uart0 fifo to be empty
		in0 a,(UART0_LSR)
		and 0x60      ; either TEMT or THRE (fifo empty, but transmit shift register can be active)
		jr z, @not_ready3

	ld de,-1
	ld b,16 ; FIFO size
	@fifo_fill_loop:
		; push a byte
		ld hl,(data_pos)
		ld a,(hl)
		out0 (UART0_THR),a
		inc hl
		ld (data_pos),hl

		; decrement count
		ld hl,(data_count)
		or a
		adc hl,de
		ld (data_count),hl
		jr z,@reached_eof
		djnz @fifo_fill_loop
	jr @not_finished

	@reached_eof:
	; disable interrupt since send is finished
	in0 a,(UART0_IER)
	and 0xfd
	out0 (UART0_IER),a

	pop hl
	pop de
	pop bc
	push hl
	; call txend_callback
	ld hl,(txend_callback)
	ex (sp),hl
	ret

@not_finished:
	pop hl
	pop de
	pop bc
	ret

async_vdu_exit:
	ld hl,old_uart_isr+1
	ld hl,(hl)
	ld e,0x18
	ld a,0x14
	rst.lil $8
	ret
