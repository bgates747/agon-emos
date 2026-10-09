#include <stdio.h>
#include <string.h>
#include "probe.h"
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--version")) {
        puts(PROBE_NAME ": development MOSlet; requires EMOS ext.uartdiag v1");
        return 0;
    }
    if (argc != 1) return 19;
    return PROBE_RUN() ? 0 : 15;
}
