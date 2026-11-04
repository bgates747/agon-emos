#include "emos_z80_io.h"
#define SETREG(reg,bits) ((reg)|=(uint8_t)(bits))
#define RESETREG(reg,bits) ((reg)&=(uint8_t)~(uint8_t)(bits))
