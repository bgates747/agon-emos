#ifndef EMOS_UART_PROBE_H
#define EMOS_UART_PROBE_H

/* Explicit Core diagnostics, not public register or application transport interfaces. */
int emos_uart_probe(void);
int emos_general_poll(void);
int emos_visible_text(void);

#endif
