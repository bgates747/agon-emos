/* Private candidate ext.sdlink owner; no public MOS/raw GPIO ABI. */
#ifndef EMOS_PARALLEL_UPLOAD_H
#define EMOS_PARALLEL_UPLOAD_H
#include "defines.h"
BYTE emos_parallel_upload_owned(void);
BYTE emos_parallel_upload_begin(UINT32 length);
BYTE emos_parallel_upload_read(BYTE *buffer, UINT24 length);
BYTE emos_parallel_upload_end(BYTE abort);
void emos_parallel_upload_abandon(void);
#endif
