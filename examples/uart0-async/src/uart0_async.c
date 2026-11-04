#include "uart0_async.h"
const void *u0_source;
uint24_t u0_count;
void (*u0_callback)(void);
extern uint8_t u0_write_call(void);
uint8_t u0_submit(const void *source,uint24_t length,void (*callback)(void)){
 u0_source=source;u0_count=length;u0_callback=callback;return u0_write_call();
}
