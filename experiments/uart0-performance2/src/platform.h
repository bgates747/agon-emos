#ifndef PERF_PLATFORM_H
#define PERF_PLATFORM_H
#include <stdint.h>
#define TICK_HZ 1152000UL
#define WALL_HZ 72000UL
extern const uint8_t *tm_source;
extern uint24_t tm_length;
extern void (*tm_callback)(void);
uint8_t tm_write(void),tm_finish(void),rom_byte0(void);
int clocks_start(void);
void clocks_stop(void);
uint32_t ticks(void),wall_ticks(void);
#endif
