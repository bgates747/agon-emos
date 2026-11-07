/* INTEG-024 optional app-owned UART1 service; no resident source/count/queue. */
#ifndef EMOS_NETA_H
#define EMOS_NETA_H
#include "emos.h"
UINT24 emos_neta_gateway(t_emosGatewayRequest*);
/* Private scoped foreground exclusion. False-IFF entry is a pure refusal;
 * the paired native tail keeps receiver STOP until immediately before EI. */
extern volatile BYTE emos_neta_rx_hold;
BYTE emos_neta_lock(void);
void emos_neta_unlock(BYTE);
BYTE emos_neta_rx_resume(void);
BYTE emos_neta_open(UINT24);
BYTE emos_neta_start(void);
BYTE emos_neta_progress(void);
BYTE emos_neta_close(void);
void emos_neta_abort(void);
void emos_neta_service(BYTE);
void emos_neta_fault(BYTE);
void emos_neta_tick(void);
#endif
