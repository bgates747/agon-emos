#ifndef EXAMPLE_UART0_ASYNC_H
#define EXAMPLE_UART0_ASYNC_H
#include <stdint.h>
enum { U0_OK=0,U0_BUSY=1,U0_INVALID=2 };
/* Matching fork firmware only; foreground, ADL1/MBASE0, EI. Not reentrant. */
uint8_t u0_submit(const void *source,uint24_t length,void (*callback)(void));
uint8_t u0_finish(void);
#endif
