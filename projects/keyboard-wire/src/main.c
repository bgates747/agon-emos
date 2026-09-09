/* Ordinary SD-loaded paired keyboard test. EMOS owns UART1 and all keyboard
 * effects. This application only selects the source via CLI, registers the
 * public callback and reads public sysvars/map. It never switches video mode,
 * injects keyboard state or calls a private receiver function. Missing input
 * has a ten-second clock deadline, and every exit restores mainboard input.
 */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "build_identity.h"
extern void probe_callback(void);
volatile uint8_t callback_irq_enabled;
static const volatile uint8_t *sv, *map;
static volatile uint8_t events, overflow, bad_irq;
static volatile uint8_t records[12][8];
static const uint8_t expected[12][4]={
    {'a',0,22,1},{'a',0,22,0},{0,2,117,1},
    {'B',2,49,1},{'B',2,49,1},{'B',2,49,1},
    {'B',2,49,0},{0,0,117,0},
    {'7',0,9,1},{'7',0,9,0},{13,0,143,1},{13,0,143,0}
};
void probe_record(uint8_t *p) {
    uint8_t n=events;
    if (callback_irq_enabled) bad_irq=1;
    if (n>=12) { overflow=1; return; }
    for (uint8_t i=0;i<4;++i) records[n][i]=p[i];
    records[n][4]=sv[sysvar_vkeycount];
    records[n][5]=map[0]; records[n][6]=map[8]; records[n][7]=map[12];
    events=n+1;
}
static uint8_t cli(const char *text) {
    char command[40]; strcpy(command,text);
    return mos_oscli(command,NULL,0)==0;
}
static uint8_t result_file(uint8_t result) {
    uint8_t handle=mos_fopen("/keyboard-state.bin",FA_WRITE|FA_CREATE_ALWAYS);
    if (!handle) return 0;
    uint24_t count=mos_fwrite(handle,(char *)&result,1);
    mos_fclose(handle); return count==1;
}
static uint8_t empty_map(void) {
    for (uint8_t i=0;i<16;++i) if (map[i]) return 0;
    return 1;
}
#define CHECK(x) do { if (!(x)) { failed_line=__LINE__; goto cleanup; } } while (0)
int main(void) {
    unsigned failed_line=0;
    uint8_t before,last,now;
    uint16_t elapsed=0;
    uint24_t stalled=0xFFFFFF;
    sv=mos_sysvars(); map=mos_getkbmap();
    puts("P4 KEYBOARD " PROBE_BUILD_ID " (" PROBE_STATUS ")");
    CHECK(cli("SET KEYBOARD 1"));
    CHECK(empty_map());
    before=sv[sysvar_vkeycount];
    mos_setkbvector(probe_callback,0);
    CHECK(cli("EMOS KEYINPUT browser"));
    puts("P4 KEYBOARD: waiting for twelve controlled events...");
    last=sv[sysvar_time];
    while (events<12 && !overflow && elapsed<1200 && --stalled) {
        now=sv[sysvar_time];
        if (now!=last) { elapsed+=(uint8_t)(now-last); last=now; stalled=0xFFFFFF; }
    }
    CHECK(events==12 && !overflow && !bad_irq);
    for (uint8_t i=0;i<12;++i) {
        for (uint8_t j=0;j<4;++j) CHECK(records[i][j]==expected[i][j]);
        CHECK(records[i][4]==(uint8_t)(before+i));
    }
    CHECK(records[1][6]==2 && records[2][6]==0); /* a down, then up */
    CHECK(records[3][5]==8 && records[4][7]==0x10 && records[5][7]==0x10 &&
          records[6][7]==0x10 && records[7][7]==0 && records[7][5]==8 &&
          records[8][5]==0); /* Shift and B remain independently held. */
    CHECK(empty_map() && sv[sysvar_vkeycount]==(uint8_t)(before+12) &&
          sv[sysvar_keyascii]==13 && !sv[sysvar_keymods] &&
          sv[sysvar_vkeycode]==143 && !sv[sysvar_vkeydown] && elapsed);
    puts("P4 KEYBOARD PASS: twelve packets, callbacks and counter");
    puts("P4 KEYBOARD PASS: held-key map, modifiers and repeat");
cleanup:
    mos_setkbvector(NULL,0);
    if (!cli("EMOS KEYINPUT mainboard") && !failed_line) failed_line=__LINE__;
    if (overflow && !failed_line) failed_line=__LINE__;
    if (!failed_line && !result_file(1)) failed_line=__LINE__;
    if (failed_line) {
        (void)result_file(255);
        printf("P4 KEYBOARD FAIL: check %u, events %u, IRQ %u\n",failed_line,events,bad_irq);
        return 1;
    }
    puts("P4 KEYBOARD PASS - returning to MOS");
    return 0;
}
