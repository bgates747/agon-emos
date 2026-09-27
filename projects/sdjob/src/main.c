/* Only resident admission can supply a live grant. No mode or UART changes. */
#include "job.h"
#include <agon/mos.h>
#include <string.h>
#include <stdio.h>
#ifndef SDJOB_BUILD_ID
#define SDJOB_BUILD_ID "UNVERSIONED-sdjob-DO-NOT-DEPLOY"
#define SDJOB_STATUS "development"
#endif
int sdjob_cancelled(void) {
  const volatile unsigned char *s = mos_sysvars();
  return s[sysvar_vkeydown] && s[sysvar_keyascii] == 27;
}
int main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--version")) {
    puts(SDJOB_BUILD_ID " (" SDJOB_STATUS ")");
    return 0;
  }
  if (argc != 2 || strcmp(argv[1], "--admitted"))
    return 19;
  return sdjob_run();
}
