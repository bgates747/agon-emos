/* Focused typing through PUBLIC EMOS APIs. The callback records key-downs;
 * the foreground alone prints via edu.text-probe. No UART/GPIO or mode change.
 * Five-minute limit and stalled-clock budget provide keyboard-free recovery.
 * Preview mirrors EMOS-acknowledged text to the mainboard for emulator review;
 * it never substitutes for the real UART round trip. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "build_identity.h"
extern void probe_callback(void);
extern uint24_t emos_gateway_call(uint8_t *request);
volatile uint8_t callback_irq_enabled;
static volatile uint8_t queue[64], read_at, write_at, overflow;
static uint8_t request[66];
static char text[160];
static int preview;
void probe_record(uint8_t *p) {
    if (!(p[3]&1) || !p[0]) return;
    uint8_t next=(write_at+1)&63;
    if (next==read_at || callback_irq_enabled) { overflow=1; return; }
    queue[write_at]=p[0]; write_at=next;
}
static uint8_t cli(const char *s) {
    char command[40]; strcpy(command,s); return mos_oscli(command,NULL,0)==0;
}
static void put24(uint8_t *p,uint24_t v) { p[0]=v; p[1]=v>>8; p[2]=v>>16; }
static int output(const char *value) {
    size_t n=strlen(value);
    memcpy(text,value,n);
    memset(request,0,sizeof(request)); request[0]=66; request[2]=1;
    request[4]=3; request[5]=10; request[6]=2;
    put24(request+10,(uint24_t)text); put24(request+13,n);
    memcpy(request+25,"edu",3); memcpy(request+41,"text-probe",10);
    if (emos_gateway_call(request)) return 0;
    if (preview) for (size_t i=0;i<n;++i) putch(text[i]);
    return 1;
}
int main(int argc,char **argv) {
    const volatile uint8_t *sv=mos_sysvars();
    unsigned elapsed=0, column=0, accepted=0;
    uint8_t last=sv[sysvar_time], result=0xFF;
    uint24_t stalled=0xFFFFFF;
    preview=argc>1 && !strcmp(argv[1],"preview");
    puts("BROWSER TYPING " PROBE_BUILD_ID " (" PROBE_STATUS ")");
    if (!cli("SET KEYBOARD 1")) goto cleanup;
    mos_setkbvector(probe_callback,0);
    if (!cli("EMOS KEYINPUT browser")) goto cleanup;
    if (!output("\x0c" "EMOS TO EDP: BROWSER KEYBOARD\r\nClick Capture keyboard, then type.\r\nEscape exits; five minute limit.\r\n\r\n")) goto cleanup;
    puts("BROWSER TYPING READY: use the focused browser display");
    while (elapsed<36000 && --stalled && !overflow) {
        uint8_t now=sv[sysvar_time];
        if (now!=last) { elapsed+=(uint8_t)(now-last); last=now; stalled=0xFFFFFF; }
        if (read_at==write_at) continue;
        uint8_t ch=queue[read_at]; read_at=(read_at+1)&63;
        if (ch==27) { result=1; break; }
        if (ch==13) { if (!output("\r\n")) goto cleanup; column=0; ++accepted; }
        else if (ch==8) {
            if (column) { if (!output("\x08 \x08")) goto cleanup; --column; ++accepted; }
        } else if (ch>=32 && ch<=126) {
            char one[2]={(char)ch,0};
            if (column==70) { if (!output("\r\n")) goto cleanup; column=0; }
            if (!output(one)) goto cleanup;
            ++column; ++accepted;
        }
    }
    if (elapsed>=36000) result=2;
    if (result!=0xFF && !output("\r\nTyping session ended. Reset Agon to run again.\r\n")) result=0xFF;
cleanup:
    mos_setkbvector(NULL,0);
    if (!cli("EMOS KEYINPUT mainboard")) result=0xFF;
    if (overflow) result=0xFF;
    uint8_t handle=mos_fopen("/typing-state.bin",FA_WRITE|FA_CREATE_ALWAYS);
    if (!handle) result=0xFF;
    else { if (mos_fwrite(handle,(char *)&result,1)!=1) result=0xFF; mos_fclose(handle); }
    if (result==0xFF) puts("BROWSER TYPING FAIL: transport, queue or clock; returning to MOS");
    else printf("BROWSER TYPING PASS: %u edited characters; returning to MOS\r\n",accepted);
    return result==0xFF ? 1 : 0;
}
