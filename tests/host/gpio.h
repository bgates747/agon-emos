#ifndef EMOS_UART_HOST_GPIO_H
#define EMOS_UART_HOST_GPIO_H

#include "eZ80.h"

#define PORTPIN_ZERO ((BYTE)0x01)
#define PORTPIN_ONE ((BYTE)0x02)
#define PORTPIN_TWO ((BYTE)0x04)
#define PORTPIN_THREE ((BYTE)0x08)

#define SETREG(reg, bits) ((reg) |= (BYTE)(bits))
#define RESETREG(reg, bits) ((reg) &= (BYTE)~(BYTE)(bits))

#endif
