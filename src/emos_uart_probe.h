#ifndef EMOS_UART_PROBE_H
#define EMOS_UART_PROBE_H

/* Explicit Core diagnostics, not public register or application transport interfaces. */
int emos_uart_probe(void);
int emos_general_poll(void);
int emos_visible_text(void);
#define EMOS_TEXT_LIMIT 1024
int emos_text_valid(const BYTE *text, UINT16 length);
int emos_text_probe(const BYTE *text, UINT16 length);

#endif
