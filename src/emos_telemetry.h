/* BENCH-001 resident telemetry; no application pointers survive publication. */
#ifndef EMOS_TELEMETRY_H
#define EMOS_TELEMETRY_H
#include "emos.h"
UINT24 emos_telemetry_gateway(t_emosGatewayRequest *request);
void emos_telemetry_reset(void);
#endif
