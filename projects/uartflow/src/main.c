#include <stdio.h>
#include <string.h>
#include "flow.h"
int main(int argc, char **argv) {
    if (argc==2 && !strcmp(argv[1],"--version")) {
        puts("uartflow: development MOSlet; requires EMOS ext.uartdiag v1"); return 0;
    }
    if (argc!=1) return 19;
    return emos_uart_flow() ? 0 : 15;
}
