/* INTEG-009 Work 3: ordinary SD application using public MOS APIs only.
 * The host peer observes a one-byte SD stage marker and supplies stock UART
 * packets. This test-control file is NOT a product packet ACK or saved setting.
 * No mode change, UART register/vector ownership, private EMOS call or sysvar
 * write occurs here. The callback is always cleared before returning to MOS.
 * Blocking stock getkey/editor APIs have no guest timeout; the host bounds the
 * whole test. Never run unattended on hardware without a prepared key source.
 */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "build_identity.h"

extern void probe_callback(void);
extern uint8_t probe_getkey(void);
volatile uint8_t callback_irq_enabled;
static const volatile uint8_t *sv, *map;
static volatile uint24_t events;
static volatile uint8_t edit_payload, overflow, bad_irq;
static uint8_t records[320][10];
static uint8_t stage_number;
static unsigned failed_line;

void probe_record(uint8_t *p) {
    uint24_t n = events;
    if (callback_irq_enabled) bad_irq = 1;
    if (n >= 320) { overflow = 1; return; }
    memcpy(records[n], p, 4);
    records[n][4] = sv[sysvar_keyascii];
    records[n][5] = sv[sysvar_keymods];
    records[n][6] = sv[sysvar_vkeycode];
    records[n][7] = sv[sysvar_vkeydown];
    records[n][8] = sv[sysvar_vkeycount];
    records[n][9] = map[8];
    if (edit_payload && p[2] == 45) { p[0] = 'z'; p[2] = 47; }
    events = n + 1;
}

static uint8_t mark(uint8_t stage) {
    stage_number = stage;
    uint8_t handle = mos_fopen("/keyboard-state.bin", FA_WRITE | FA_CREATE_ALWAYS);
    if (!handle) return 0;
    uint24_t count = mos_fwrite(handle, (char *)&stage, 1);
    mos_fclose(handle);
    return count == 1;
}
static uint8_t wait_events(uint24_t expected) {
    uint24_t budget = 0xFFFFFF;
    while (events < expected && --budget) {}
    return events == expected && !overflow && !bad_irq;
}
static uint8_t empty_map(void) {
    for (uint8_t i = 0; i < 16; ++i) if (map[i]) return 0;
    return 1;
}
static uint8_t cli(const char *text) {
    /* MOS may tokenize command text in place. */
    char command[40];
    strcpy(command, text);
    return mos_oscli(command, NULL, 0) == 0;
}

#define CHECK(value) do { if (!(value)) { failed_line = __LINE__; goto failed; } } while (0)
int main(void) {
    uint8_t before, old_ascii, old_mods, old_vk, old_down, clock_before, flags_before;
    uint24_t base, budget;
    char line[16];
    sv = mos_sysvars(); map = mos_getkbmap();
    puts("KEYBOARD API " PROBE_BUILD_ID " (draft)");
    CHECK(cli("SET KEYBOARD 1"));
    CHECK(cli("EMOS KEYINPUT browser"));
    CHECK(empty_map());
    mos_setkbvector(probe_callback, 0);
    before = sv[sysvar_vkeycount]; old_ascii = sv[sysvar_keyascii];
    old_mods = sv[sysvar_keymods]; old_vk = sv[sysvar_vkeycode];
    old_down = sv[sysvar_vkeydown]; clock_before = sv[sysvar_time];

    CHECK(mark(1) && wait_events(1));
    CHECK(!memcmp(records[0], (uint8_t[]){'a',0,22,1}, 4));
    CHECK(records[0][4] == old_ascii && records[0][5] == old_mods &&
          records[0][6] == old_vk && records[0][7] == old_down &&
          records[0][8] == before && records[0][9] == 0);
    CHECK(sv[sysvar_keyascii] == 'a' && sv[sysvar_vkeycode] == 22 &&
          sv[sysvar_vkeydown] == 1 && map[8] == 2 &&
          sv[sysvar_vkeycount] == (uint8_t)(before+1));
    CHECK(mark(2) && wait_events(3)); /* b down, a up: b remains held */
    CHECK(!map[8] && map[12] == 0x10 && sv[sysvar_vkeydown] == 0 &&
          sv[sysvar_keymods] == 2);
    CHECK(mark(3) && wait_events(4) && empty_map());
    puts("KEYBOARD API PASS: callback order, sysvars and held-key map");

    edit_payload = 1;
    CHECK(mark(4) && wait_events(5));
    CHECK(sv[sysvar_keyascii] == 'z' && sv[sysvar_vkeycode] == 47 &&
          map[12] == 2 && !(map[8] & 4));
    CHECK(mark(5) && wait_events(6) && empty_map());
    edit_payload = 0;
    puts("KEYBOARD API PASS: callback payload edits and register preservation");

    before = sv[sysvar_vkeycount]; base = events;
    CHECK(mark(6) && wait_events(base+260));
    CHECK(sv[sysvar_vkeycount] == (uint8_t)(before+260) && empty_map());
    for (uint24_t i = 0; i < 260; ++i)
        CHECK(records[base+i][8] == (uint8_t)(before+i));
    puts("KEYBOARD API PASS: 260 packets and keycount wrap");

    before = sv[sysvar_vkeycount]; base = events;
    flags_before = sv[sysvar_vdp_pflags];
    CHECK(mark(7)); budget = 0xFFFFFF;
    while (sv[sysvar_keyled] != 5 && --budget) {}
    CHECK(budget && sv[sysvar_keydelay] == 0xF4 && sv[sysvar_keydelay+1] == 1 &&
          sv[sysvar_keyrate] == 33 && !sv[sysvar_keyrate+1] &&
          events == base && sv[sysvar_vkeycount] == before &&
          sv[sysvar_vdp_pflags] == flags_before);
    puts("KEYBOARD API PASS: five-byte settings publication");

    base = events;
    CHECK(mark(8)); CHECK(probe_getkey() == '7' && events == base+2);
    puts("KEYBOARD API PASS: mos_getkey");
    CHECK(mark(9));
    CHECK(mos_editline(line, sizeof(line), 13) == 13 && !strcmp(line, "cra"));
    puts("\nKEYBOARD API PASS: editor letters, Backspace, arrows and Enter");
    CHECK(mark(10));
    CHECK(mos_editline(line, sizeof(line), 13) == 27 && !*line);
    puts("\nKEYBOARD API PASS: editor Escape");

    /* Host finishes the Escape release before stage 11's two packets. */
    budget = 0xFFFFFF;
    while (sv[sysvar_vkeydown] && --budget) {}
    CHECK(budget);
    mos_setkbvector(NULL, 0);
    before = sv[sysvar_vkeycount]; base = events;
    CHECK(mark(11)); budget = 0xFFFFFF;
    while (sv[sysvar_vkeycount] != (uint8_t)(before+2) && --budget) {}
    CHECK(budget && events == base && empty_map());
    CHECK(cli("SET KEYBOARD 2") && cli("EMOS KEYINPUT mainboard"));
    CHECK(cli("EMOS KEYINPUT browser") && cli("EMOS KEYINPUT mainboard"));
    CHECK(sv[sysvar_time] != clock_before && !bad_irq && !overflow);
    puts("KEYBOARD API PASS: callback removed, retained layout, clock and source return");
    puts("KEYBOARD API PASS - returning to MOS");
    CHECK(mark(127));
    return 0;
failed:
    mos_setkbvector(NULL, 0);
    printf("KEYBOARD API FAIL: stage %u, check line %u, events %u, IRQ %u\n",
           stage_number, failed_line, (unsigned)events, bad_irq);
    printf("Fields: %u %u %u %u %u; map %u; callback:",
           sv[5], sv[6], sv[23], sv[24], sv[25], map[8]);
    for (uint8_t i = 0; i < 10; ++i) printf(" %u", records[0][i]);
    puts("");
    (void)cli("EMOS KEYINPUT mainboard");
    (void)mark(255);
    return 1;
}
