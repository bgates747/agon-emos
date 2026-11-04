#ifndef EMOS_UART_FLOW_H
#define EMOS_UART_FLOW_H
#include "emos.h"
/* Private transport service for SD diagnostic MOSlets, not the test routine. */
UINT24 emos_read24(const BYTE *data); /* existing resident byte-array helper */
UINT24 emos_uartdiag_gateway(t_emosGatewayRequest *request);
void emos_uartdiag_reset(void);
#endif
