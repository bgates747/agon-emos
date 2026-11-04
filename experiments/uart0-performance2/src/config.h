#ifndef PERF2_CONFIG_H
#define PERF2_CONFIG_H
#include <stdint.h>
/* Defined in a separate object: all fixtures compile the identical core. */
struct FixtureConfig {
 const char *id,*command,*name,*build;
 unsigned transport,mos_bytes;
 uint32_t mos_crc;
};
extern const struct FixtureConfig fixture;
/* 0: ordinary synchronous output; 1: minimal Eirene; 2: previous full UTX1. */
int legacy_init(void),legacy_cleanup(void);
#endif
