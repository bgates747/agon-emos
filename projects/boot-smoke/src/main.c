/* QUAL-002 ordinary MOS application: no Extender API, UART1 or GPIO access.
 * Built with the EMOS review bundle identity; the identical binary also runs
 * under stock MOS. The finite polling budget is a stalled-clock escape, not
 * a calibrated time measurement. Hardware observation has its own deadline.
 */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "build_identity.h"

int main(void) {
    static const char expected[] = "EMOS SD CHECK\r\n";
    char buffer[sizeof(expected)] = {0};
    puts("EMOS BOOT SMOKE " EMOS_BUILD_ID " (" EMOS_ARTIFACT_STATUS ")");
    const uint8_t handle = mos_fopen("/emos-boot/check.txt", FA_READ);
    if (!handle) {
        puts("BOOT SMOKE FAIL: SD open");
        return 1;
    }
    const uint24_t count = mos_fread(handle, buffer, sizeof(buffer));
    mos_fclose(handle);
    if (count != sizeof(expected) - 1 ||
        memcmp(buffer, expected, sizeof(expected) - 1) != 0) {
        puts("BOOT SMOKE FAIL: SD contents");
        return 1;
    }
    puts("BOOT SMOKE SD PASS");

    /* Only one byte is sampled, so no torn multibyte clock read is possible.
     * We require an observed change, not a particular centisecond increment. */
    const volatile uint8_t *ticks = mos_sysvars();
    const uint8_t before = ticks[sysvar_time];
    uint24_t remaining = 0xFFFFFF;
    while (ticks[sysvar_time] == before && --remaining) {}
    if (!remaining) {
        puts("BOOT SMOKE FAIL: MOS clock did not advance");
        return 1;
    }
    puts("BOOT SMOKE CLOCK PASS");
    puts("BOOT SMOKE PASS - returning to MOS");
    return 0;
}
