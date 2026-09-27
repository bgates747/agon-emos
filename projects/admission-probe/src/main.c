/* A04 finite dispatch probe; no transfers. Writes only its fixed result file.
 * A host-created sentinel is LOADed at 40000 before the editor accepts work.
 * Never install this as the real sdjob utility: it cannot perform transfers. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    char line[24];
    char nested[]="EMOS sdjob --admitted";
    unsigned i;
    const volatile uint8_t *p=(void *)0x40000;
    if(argc!=2 || strcmp(argv[1],"--admitted"))return 19;
    for(i=0;i<4096;i++)if(p[i]!=(uint8_t)(i*17+3))return 1;
    puts("Application memory preserved");
    if(mos_oscli(nested,NULL,0)!=31)return 1;
    puts("Application editor waiting");
    if(mos_editline(line,sizeof(line),1)!=13 || strcmp(line,"proof"))return 1;
    {
        char result[]="A04 memory nested-editor PASS\n";
        if(mos_save("/agents/extender/results/a04-probe.bin",result,sizeof(result)-1))return 1;
    }
    puts("\nFinite admission fixture returned");
    return 0;
}
