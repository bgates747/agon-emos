/* Candidate application controller service; resident UART ownership stays EMOS. */
#ifndef EMOS_CONTROLLER_H
#define EMOS_CONTROLLER_H
#include "emos.h"
UINT24 emos_controller_gateway(t_emosGatewayRequest *r);
void emos_controller_packet(const BYTE *p,BYTE length);
void emos_controller_reset(void);
void emos_controller_tick(void);
#endif
