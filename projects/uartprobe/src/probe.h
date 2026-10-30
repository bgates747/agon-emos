#pragma once
#include "flow.h"
int emos_uart_probe(void);
int emos_general_poll(void);
#define EMOS_TEXT_LIMIT 1024
int emos_visible_text(void);
int emos_text_valid(const BYTE *text, UINT16 length);
int emos_text_probe(const BYTE *text, UINT16 length);
