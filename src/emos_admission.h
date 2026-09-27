/* REMOTE-005 private idle-CLI admission. Not a public MOS editor/API flag. */
#ifndef EMOS_ADMISSION_H
#define EMOS_ADMISSION_H
#include <defines.h>
#define EMOS_CLI_SERVICE 254
BYTE emos_admission_idle(BYTE observed_keycount);
void emos_admission_leave(void);
void emos_admission_reset(void);
void emos_admission_packet(const BYTE *data, BYTE length);
BYTE emos_admission_dispatching(void);
int emos_admission_dispatch(void);
/* Owned by emos.c, reusing its policy and checked MOSlet loader. */
BYTE emos_admission_core(void);
int emos_admission_run(void);
#endif
