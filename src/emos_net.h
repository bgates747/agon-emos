/* Fixed admitted network delivery; application policy stays outside EMOS. */
#ifndef EMOS_NET_H
#define EMOS_NET_H
#include "emos.h"
UINT24 emos_net_gateway(t_emosGatewayRequest*);
void emos_net_packet(const BYTE*,BYTE);
void emos_net_reset(void);
void emos_reply_callback(const BYTE*);
#endif
