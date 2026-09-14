/*
 * Title:			AGON MOS - UART code
 * Author:			Dean Belfield
 * Created:			06/07/2022
 * Last Updated:	08/04/2023
 * 
 * Modinfo:
 * 03/08/2022:		Enabled UART0 receive interrupt
 * 08/08/2022:		Enabled UART0 CTS port
 * 22/03/2023:		Moved putch and getch to serial.asm
 * 23/03/2023:		Fixed maths overflow in init_UART0 to work with bigger baud rates
 * 28/03/2023:		Added support for UART1
 * 08/04/2023:		Interrupts now disabled in close_UART1
 *
 * NB:
 * The UART is on Port D
 *
 * - 0: RX
 * - 1: TX
 * - 2: RTS (the CTS input of the ESP32
 * - 3: CTS (the RTS output of the ESP32)
 */
 
#include <stddef.h>
#include <stdio.h>
#include <eZ80.h>
#include <defines.h>
#include <gpio.h>

#include "emos_parallel.h"
#include "uart.h"
#include "emos_keyboard.h"

volatile BYTE uart1_keyboard_owned;

static BYTE uart1_rts_owned;
 
// Set the Line Control Register for data, stop and parity bits
//
#define SETREG_LCR0(data, stop, parity) (UART0_LCTL = ((BYTE)(((data)-(BYTE)5)&(BYTE)0x3)|(BYTE)((((stop)-(BYTE)0x1)&(BYTE)0x1)<<(BYTE)0x2)|(BYTE)((parity)<<(BYTE)0x3)))
#define SETREG_LCR1(data, stop, parity) (UART1_LCTL = ((BYTE)(((data)-(BYTE)5)&(BYTE)0x3)|(BYTE)((((stop)-(BYTE)0x1)&(BYTE)0x1)<<(BYTE)0x2)|(BYTE)((parity)<<(BYTE)0x3)))

void init_UART0() {
	PD_DR = PORTD_DRVAL_DEF;
	PD_DDR = PORTD_DDRVAL_DEF;
//	#ifdef _EZ80F91
//	PD_ALT0 = PORTD_ALT0VAL_DEF;
//	#endif
	PD_ALT1 = PORTD_ALT1VAL_DEF;
	PD_ALT2 = PORTD_ALT2VAL_DEF;
	return ;
}

void init_UART1() {
	PC_DR = PORTC_DRVAL_DEF;
	PC_DDR = PORTC_DDRVAL_DEF;
//	#ifdef _EZ80F91
//	PC_ALT0 = PORTC_ALT0VAL_DEF;
//	#endif
	PC_ALT1 = PORTC_ALT1VAL_DEF;
	PC_ALT2 = PORTC_ALT2VAL_DEF;
	return ;
}

// Open UART0
// Parameters:
// - pUART: Structure containing the initialisation data
//
BYTE open_UART0(UART * pUART) {
	UINT32	mc = MASTERCLOCK;										// UART baud rate calculation
	/*
	 * AgonDev evaluates the inherited product below at the eZ80 native
	 * 24-bit width unless an operand is widened before multiplication. At
	 * 1,152,000 baud that wrapped 0x01194000 to 0x00194000 and programmed
	 * BRG divisor 11 instead of 1 on physical hardware. Keep the upstream
	 * formula, but make its intended 32-bit intermediate explicit.
	 */
	UINT32	cb = (UINT32)CLOCK_DIVISOR_16 * (UINT32)pUART->baudRate;	// split to avoid eZ80 maths overflow error
	UINT32	br = mc / cb;											// with larger baud rate values

	UCHAR	pins = PORTPIN_ZERO | PORTPIN_ONE;						// The transmit and receive pins											

	serialFlags &= 0xF0;
	
	SETREG(PD_DDR, pins);											// Set Port D bits 0, 1 (TX. RX) for alternate function.
	RESETREG(PD_ALT1, pins);
	SETREG(PD_ALT2, pins);

	if (pUART->flowControl == FCTL_HW) {
		SETREG(PD_DDR, PORTPIN_THREE | PORTPIN_TWO);				// Set Port D bit 3 and 2 (CTS,RTS) to alternate function
		RESETREG(PD_ALT1, PORTPIN_THREE | PORTPIN_TWO);
		SETREG(PD_ALT2, PORTPIN_THREE | PORTPIN_TWO);
		serialFlags |= 0x02;
	}

	UART0_LCTL |= UART_LCTL_DLAB;									// Select DLAB to access baud rate generators
	UART0_BRG_L = (br & 0xFF);										// Load divisor low
	UART0_BRG_H = (CHAR)(( br & 0xFF00 ) >> 8);						// Load divisor high
	UART0_LCTL &= (~UART_LCTL_DLAB); 								// Reset DLAB; dont disturb other bits
	UART0_MCTL = 0x02;												// Multidrop, loopback, DTR disabled, RTS enabled
	UART0_FCTL = 0x07;												// Enable and clear hardware FIFOs
	UART0_IER = pUART->interrupts;									// Set interrupts
	
	SETREG_LCR0(pUART->dataBits, pUART->stopBits, pUART->parity);	// Set the line status register

	serialFlags |= 0x01;
	
	return UART_ERR_NONE;
}

// Open UART1
// Parameters:
// - pUART: Structure containing the initialisation data
//
BYTE open_UART1(UART * pUART) {
	UINT32	mc = MASTERCLOCK;										// UART baud rate calculation
	UINT32	cb = (UINT32)CLOCK_DIVISOR_16 * (UINT32)pUART->baudRate;	// preserve the same widened intermediate as UART0
	UINT32	br = mc / cb;											// with larger baud rate values

	UCHAR	pins = PORTPIN_ZERO | PORTPIN_ONE;						// The transmit and receive pins

	/* EMOS and UART1 share Port C.  Reserve the production lifecycle lock
	 * before the first flag, mux, or UART mutation; a committed parallel epoch
	 * is indefinite, so contention is rejected rather than spun on. */
	if (uart1_keyboard_owned) return UART_ERR_FAILURE;
	if (emos_parallel_uart1_guard_acquire() != EMOS_PARALLEL_OK)
		return UART_ERR_FAILURE;

	serialFlags &= 0x0F;

	SETREG(PC_DDR, pins);											// Set Port C bits 0, 1 (TX. RX) for alternate function.
	RESETREG(PC_ALT1, pins);
	SETREG(PC_ALT2, pins);

	if (pUART->flowControl == FCTL_HW) {
		SETREG(PC_DDR, PORTPIN_THREE);								// Set Port C bit 3 (CTS) for input
		RESETREG(PC_ALT1, PORTPIN_THREE);
		RESETREG(PC_ALT2, PORTPIN_THREE);
		serialFlags |= 0x20;
	}
	
	UART1_LCTL |= UART_LCTL_DLAB;									// Select DLAB to access baud rate generators
	UART1_BRG_L = (br & 0xFF);										// Load divisor low
	UART1_BRG_H = (CHAR)(( br & 0xFF00 ) >> 8);						// Load divisor high
	UART1_LCTL &= (~UART_LCTL_DLAB); 								// Reset DLAB; dont disturb other bits
	UART1_MCTL = 0x00;												// Bring modem control register to reset value
	UART1_FCTL = 0x07;												// Enable and clear hardware FIFOs
	UART1_IER = pUART->interrupts;									// Set interrupts

	serialFlags |= 0x10;
	
	SETREG_LCR1(pUART->dataBits, pUART->stopBits, pUART->parity);	// Set the line status register

	/* Publish UART1 active before releasing the reservation.  An interrupting
	 * epoch therefore observes either the held lock or serialFlags bit 4. */
	emos_parallel_uart1_guard_release();
	
	return UART_ERR_NONE;
}

// Close UART1
//
void close_UART1() {
    if (uart1_keyboard_owned) return; /* Public close cannot steal the receiver. */
    if (uart1_rts_owned) {
        SETREG(PC_DR, PORTPIN_TWO);  /* Stop peer before abandoning receive. */
        UART1_FCTL = 0x07;          /* Cancel queued data owned by this probe. */
        SETREG(PC_DDR, PORTPIN_TWO); /* Return added RTS driver to GPIO input. */
        uart1_rts_owned = 0;
    }
	UART1_IER = 0x00;												// Disable UART1 interrupts
	UART1_LCTL = 0x00; 												// Bring line control register to reset value.
	UART1_MCTL = 0x00;												// Bring modem control register to reset value.
	UART1_FCTL = 0x00;												// Bring FIFO control register to reset value.	
	serialFlags &= 0x0F;
}


/* INTEG-005: explicit Core ownership of PC2, separate from stock UART1's
 * CTS-only FCTL_HW option. Never seize an output/alternate-function pin.
 * Active-low ready is preloaded HIGH (stop) before enabling the driver.
 */
BYTE uart1_claim_rts(void) {
    if (uart1_keyboard_owned || uart1_rts_owned || (serialFlags & 0x30) != 0x30 || UART1_IER != 0 ||
        !(PC_DDR & PORTPIN_TWO) || ((PC_ALT1 | PC_ALT2) & PORTPIN_TWO))
        return UART_POLL_UNAVAILABLE;
    SETREG(PC_DR, PORTPIN_TWO);
    RESETREG(PC_DDR, PORTPIN_TWO);
    uart1_rts_owned = 1;
    return UART_POLL_READY;
}

BYTE uart1_receive_ready(BYTE ready) {
    if (uart1_keyboard_owned || !uart1_rts_owned || (serialFlags & 0x30) != 0x30 || UART1_IER != 0)
        return UART_POLL_UNAVAILABLE;
    if (ready) RESETREG(PC_DR, PORTPIN_TWO);
    else SETREG(PC_DR, PORTPIN_TWO);
    return UART_POLL_READY;
}

/* Nonblocking Core operations for an open polling UART.
 * Reject interrupt readers; honor active-low GPIO CTS when FCTL_HW is set.
 * The stock blocking APIs retain their existing behavior. Reading LSR acknowledges receive error flags;
 * report errors before consuming data, and never return stale data as valid.
 */
BYTE uart1_try_get(BYTE *value) {
    BYTE status;
    if (uart1_keyboard_owned || !value || !(serialFlags & 0x10) || UART1_IER != 0)
        return UART_POLL_UNAVAILABLE;
    status = UART1_LSR;
    if (status & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_ERR))
        return UART_POLL_ERROR;
    if (!(status & UART_LSR_DR)) return UART_POLL_EMPTY;
    *value = UART1_RBR;
    return UART_POLL_READY;
}

BYTE uart1_try_put(BYTE value) {
    BYTE status;
    if (uart1_keyboard_owned || !(serialFlags & 0x10) || UART1_IER != 0)
        return UART_POLL_UNAVAILABLE;
    status = UART1_LSR;
    /* LSR reads also clear RX error flags: do not silently lose them here. */
    if (status & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_ERR))
        return UART_POLL_ERROR;
    if ((serialFlags & 0x20) && (PC_DR & PORTPIN_THREE))
        return UART_POLL_BLOCKED;
    if (!(status & UART_LSR_THRE)) return UART_POLL_EMPTY;
    UART1_THR = value;
    return UART_POLL_READY;
}

/* INTEG-009: this resident driver has a distinct ownership path. Open/close
 * run with interrupts masked by the coordinator; public UART calls retain
 * their old behavior unless this driver owns UART1. Do not relax the polling
 * helpers' IER checks to share their receive/error side effects. */
extern BYTE emos_keyboard_vector(BYTE operation);

BYTE uart1_keyboard_open(void) {
    UART settings = {1152000, 8, 1, 0, FCTL_HW, 0};
    if (uart1_keyboard_owned || uart1_rts_owned || (serialFlags & 0x10) ||
        UART1_IER || (PC_DDR & 0x0F) != 0x0F || (PC_ALT1 & 0x0F) ||
        (PC_ALT2 & 0x0C) || ((PC_ALT2 & 3) != 0 && (PC_ALT2 & 3) != 3))
        return UART_POLL_UNAVAILABLE;
    if (!emos_keyboard_vector(1)) return UART_POLL_UNAVAILABLE;
    if (open_UART1(&settings) != UART_ERR_NONE) {
        emos_keyboard_vector(0); return UART_POLL_UNAVAILABLE;
    }
    if (uart1_claim_rts() != UART_POLL_READY) {
        close_UART1(); emos_keyboard_vector(0); return UART_POLL_UNAVAILABLE;
    }
    uart1_keyboard_owned = 1;
    UART1_FCTL = 0x07;
    UART1_IER = UART_IER_RECEIVEINT | UART_IER_LINESTATUSINT;
    RESETREG(PC_DR, PORTPIN_TWO); /* Receiver/vector ready before admission. */
    return UART_POLL_READY;
}
void uart1_keyboard_stop(void) {
    if (!uart1_keyboard_owned) return;
    SETREG(PC_DR, PORTPIN_TWO);
    UART1_IER = 0;
}
void uart1_keyboard_close(void) {
    if (!uart1_keyboard_owned) return;
    /* A hostile/raw replacement is unsupported, but never overwrite it. */
    if (!emos_keyboard_vector(2)) return;
    uart1_keyboard_stop();
    uart1_keyboard_owned = 0;
    close_UART1();
    /* Release only r03 PC0..PC3. Other Port C lanes retain their state. */
    SETREG(PC_DDR, 0x0F);
    RESETREG(PC_ALT1, 0x0F);
    RESETREG(PC_ALT2, 0x0F);
    emos_keyboard_vector(0);
}
/* Target leaf is in emos_keyboard_io.asm; retain the original C oracle. */
#ifdef EMOS_UART_PUT_C_REFERENCE
BYTE uart1_keyboard_put(BYTE value) {
    BYTE irq = emos_keyboard_lock(), status, result;
    if (!uart1_keyboard_owned || emos_key_faulted) result = UART_POLL_UNAVAILABLE;
    else {
        status = UART1_LSR;
        if (status & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_ERR)) {
            uart1_keyboard_stop(); /* Preserve acknowledged errors for caller. */
            result = UART_POLL_ERROR;
        } else if (PC_DR & PORTPIN_THREE) result = UART_POLL_BLOCKED;
        else if (!(status & UART_LSR_THRE)) result = UART_POLL_EMPTY;
        else { UART1_THR = value; result = UART_POLL_READY; }
    }
    emos_keyboard_unlock(irq);
    return result;
}
#endif
void uart1_keyboard_irq(void) {
    BYTE count, status;
    if (!uart1_keyboard_owned || emos_key_faulted) return;
    SETREG(PC_DR, PORTPIN_TWO); /* Stop peer while processing its FIFO. */
    for (count = 0; count < 16; ++count) {
        status = UART1_LSR;
        if (status & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_ERR)) {
            emos_keyboard_fault(); return;
        }
        if (!(status & UART_LSR_DR)) break;
        emos_keyboard_byte(UART1_RBR);
        if (emos_key_faulted) return;
    }
    RESETREG(PC_DR, PORTPIN_TWO);
#ifdef EMOS_BENCH_TELEMETRY
    emos_keyboard_async_irq(); /* RX remains first; bounded TX next. */
#endif
}
#ifdef EMOS_BENCH_TELEMETRY
void uart1_keyboard_tx_enable(BYTE enabled) {
    if (!uart1_keyboard_owned || emos_key_faulted) return;
    if (enabled) UART1_IER |= UART_IER_TRANSMITINT;
    else UART1_IER &= (BYTE)~UART_IER_TRANSMITINT;
}
#endif
