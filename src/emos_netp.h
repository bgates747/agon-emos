/* INTEG-020: foreground application admission; no app pointer/ISR mailbox. */
#ifndef EMOS_NETP_H
#define EMOS_NETP_H
#include "emos.h"
#define EMOS_NETP_SMALL 130
#define EMOS_NETP_EMPTY 131
#define EMOS_NETP_FAULT 132
#define EMOS_NETP_MAX 142
UINT24 emos_netp_gateway(t_emosGatewayRequest *);
BYTE emos_netp_owned(void);
#endif
