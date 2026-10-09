#pragma once
#include <stdint.h>
typedef uint8_t BYTE;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
#ifndef UART_POLL_EMPTY
enum { UART_POLL_EMPTY=0, UART_POLL_READY=1, UART_POLL_ERROR=2,
       UART_POLL_UNAVAILABLE=3, UART_POLL_BLOCKED=4 };
#endif
BYTE emos_uart_flow_clock(void);
BYTE flow_open(void);
BYTE flow_get(BYTE *value);
BYTE flow_put(BYTE value);
BYTE flow_ready(BYTE value);
void flow_close(void);
int emos_uart_flow(void);
