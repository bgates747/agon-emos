#ifndef EMOS_PARALLEL_HOST_EZ80_H
#define EMOS_PARALLEL_HOST_EZ80_H

#include "defines.h"

extern volatile BYTE hostPcDr;
extern volatile BYTE hostPcDdr;
extern volatile BYTE hostPcAlt1;
extern volatile BYTE hostPcAlt2;
extern volatile BYTE hostPdDr;
extern volatile BYTE hostPdDdr;
extern volatile BYTE hostPdAlt1;
extern volatile BYTE hostPdAlt2;

#define PC_DR hostPcDr
#define PC_DDR hostPcDdr
#define PC_ALT1 hostPcAlt1
#define PC_ALT2 hostPcAlt2
#define PD_DR hostPdDr
#define PD_DDR hostPdDdr
#define PD_ALT1 hostPdAlt1
#define PD_ALT2 hostPdAlt2

#endif
