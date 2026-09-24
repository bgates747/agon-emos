/* AUDIT-008 local-only launcher probe. No UART or mode changes. The host
 * harness seeds 4096 application bytes at 40000 before launching this MOSlet.
 * A second build at 40000 exercises application-call rejection instead. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned invocations;
static uint8_t command(char *text) { return mos_oscli(text,NULL,0); }
int main(int argc,char **argv) {
    char nested[]="emos probe nested";
    char status[]="EmOs StAtUs";
    unsigned i;
    int app=argc>1 && !strcmp(argv[1],"application");
    printf("Utility probe: argc=%d invocation=%u\n",argc,++invocations);
    for(i=0;i<(unsigned)argc;++i) printf("arg%u=[%s]\n",i,argv[i]);
    if(!app) {
        const volatile uint8_t *memory=(void *)0x40000;
        for(i=0;i<4096;++i) if(memory[i]!=(uint8_t)(i*17+3)) {
            puts("FAIL: application sentinel changed");return 1;
        }
        puts("PASS: application sentinel preserved");
    }
    if(command(nested)!=31) { puts("FAIL: nested utility admitted");return 1; }
    puts(app?"PASS: application nested dispatch rejected":"PASS: MOSlet nested dispatch rejected");
    if(command(status)!=0) { puts("FAIL: resident status rejected");return 1; }
    puts("PASS: resident built-in remains available");
    if(argc>1 && !strcmp(argv[1],"return7")) return 7;
    return 0;
}
