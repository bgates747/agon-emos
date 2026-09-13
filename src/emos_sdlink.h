/* INTEG-013: owned foreground transport, never filesystem work in an ISR. */
#ifndef EMOS_SDLINK_H
#define EMOS_SDLINK_H
#include "emos.h"
#define EMOS_SDLINK_LIMIT 240
/* Private Core helper shared with the gateway; not a MOS API slot. */
UINT24 emos_read24(const BYTE *data);
UINT24 emos_sdlink_gateway(t_emosGatewayRequest *request);
void emos_sdlink_packet(const BYTE *data, BYTE length);
void emos_sdlink_reset(void);
#endif
