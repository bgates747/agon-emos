/* Resident ordinary-console adapter; private to the EMOS coordinator. */
#ifndef EMOS_CONSOLE_H
#define EMOS_CONSOLE_H
#include <defines.h>
extern volatile BYTE emos_console_owned;
BYTE emos_console_prepare(void);
BYTE emos_console_commit(void);
BYTE emos_console_recover(void);
void emos_console_publish(void);
void emos_console_packet(BYTE command, BYTE *payload, BYTE length);
BYTE emos_console_write_byte(BYTE value);
BYTE emos_console_write_stream(const BYTE *data, UINT16 length);
void emos_console_notice(BYTE exclusive);
void emos_vdp_effect(BYTE command);
void emos_mainboard_put(BYTE value);
#endif
