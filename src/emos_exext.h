/* PORT-008 ordinary ExExt adapter. Private EMOS implementation, no app ABI. */
#ifndef EMOS_EXEXT_H
#define EMOS_EXEXT_H
#include <defines.h>
extern volatile BYTE emos_exext_owned, emos_exext_busy;
BYTE emos_exext_start(void);
BYTE emos_exext_stop(void);
void emos_exext_tick(void);
void emos_exext_interrupt_service(void);
#endif
