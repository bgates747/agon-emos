#ifndef EMOS_UART_HOST_EZ80_H
#define EMOS_UART_HOST_EZ80_H

#include "ez80.h"

extern volatile BYTE hostUart0BrgL;
extern volatile BYTE hostUart0BrgH;
extern volatile BYTE hostUart0Lctl;
extern volatile BYTE hostUart0Mctl;
extern volatile BYTE hostUart0Fctl;
extern volatile BYTE hostUart0Ier;
extern volatile BYTE hostUart1BrgL;
extern volatile BYTE hostUart1BrgH;
extern volatile BYTE hostUart1Lctl;
extern volatile BYTE hostUart1Mctl;
extern volatile BYTE hostUart1Fctl;
extern volatile BYTE hostUart1Ier;
extern volatile BYTE hostUart1Lsr;
extern volatile BYTE hostUart1Rbr;
extern volatile BYTE hostUart1Thr;

#define UART0_BRG_L hostUart0BrgL
#define UART0_BRG_H hostUart0BrgH
#define UART0_LCTL hostUart0Lctl
#define UART0_MCTL hostUart0Mctl
#define UART0_FCTL hostUart0Fctl
#define UART0_IER hostUart0Ier
#define UART1_BRG_L hostUart1BrgL
#define UART1_BRG_H hostUart1BrgH
#define UART1_LCTL hostUart1Lctl
#define UART1_MCTL hostUart1Mctl
#define UART1_FCTL hostUart1Fctl
#define UART1_IER hostUart1Ier
#define UART1_LSR hostUart1Lsr
#define UART1_RBR hostUart1Rbr
#define UART1_THR hostUart1Thr

#define PORTC_DRVAL_DEF ((BYTE)0xFF)
#define PORTC_DDRVAL_DEF ((BYTE)0xFF)
#define PORTC_ALT1VAL_DEF ((BYTE)0x00)
#define PORTC_ALT2VAL_DEF ((BYTE)0x00)
#define PORTD_DRVAL_DEF ((BYTE)0xFF)
#define PORTD_DDRVAL_DEF ((BYTE)0xFF)
#define PORTD_ALT1VAL_DEF ((BYTE)0x00)
#define PORTD_ALT2VAL_DEF ((BYTE)0x00)

#endif
