/*
 * FWBUG-008 physical oracle.
 *
 * Sector 2 is touched only after an MBR proves that the first partition starts
 * later.  The original sector is retained in RAM, the repaired RST 08h API
 * writes a derived pattern, the ordinary read API verifies it, and MOS's
 * independently dispatched C SD_writeBlocks entry restores the preimage.
 * Evidence is written only after restoration has been read back byte-for-byte.
 */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t (*control_write_t)(uint32_t, uint8_t *, uint16_t);
extern control_write_t fwbug008_control_write(void);

static uint8_t mbr[512], before[512], pattern[512], observed[512], restored[512];
static const char recovery_startup[] =
    "SET KEYBOARD 1\r\n"
    "EMOS KEYINPUT extender\r\n"
    "VDU 22 3\r\n"
    "EMOS sdserve --fast /\r\n";

static uint32_t le32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
         (uint32_t)p[3] << 24;
}

static uint32_t crc32(const uint8_t *p, unsigned n) {
  uint32_t value = 0xffffffffUL;
  while (n--) {
    value ^= *p++;
    for (unsigned bit = 0; bit != 8; ++bit)
      value = (value >> 1) ^ ((value & 1) ? 0xedb88320UL : 0);
  }
  return value ^ 0xffffffffUL;
}

static int save_result(const char *status, unsigned test_rc, unsigned restore_rc,
                       unsigned verify_rc, uint32_t before_crc,
                       uint32_t pattern_crc, uint32_t observed_crc,
                       uint32_t restored_crc) {
  FIL file = {0};
  char record[384];
  int n = snprintf(record, sizeof record,
      "schema=1\ncase=a10-rp04-raw-sd-write\nstatus=%s\nsector=2\n"
      "test_rc=%u\nrestore_rc=%u\nrestore_verify_rc=%u\n"
      "before_crc32=%08lx\npattern_crc32=%08lx\n"
      "observed_crc32=%08lx\nrestored_crc32=%08lx\n",
      status, test_rc, restore_rc, verify_rc,
      (unsigned long)before_crc, (unsigned long)pattern_crc,
      (unsigned long)observed_crc, (unsigned long)restored_crc);
  if (n < 0 || n >= (int)sizeof record ||
      ffs_fopen(&file, "/agents/extender/results/a10-rp04.txt",
                FA_WRITE | FA_CREATE_ALWAYS))
    return 19;
  int okay = ffs_fwrite(&file, record, (uint24_t)n) == (uint24_t)n;
  okay = ffs_fsync(&file) == 0 && okay;
  okay = ffs_fclose(&file) == 0 && okay;
  return okay ? 0 : 19;
}

static int arm_recovery(void) {
  FIL file = {0};
  if (ffs_fopen(&file, "/autoexec.txt", FA_WRITE | FA_CREATE_ALWAYS))
    return 0;
  uint24_t size = (uint24_t)(sizeof recovery_startup - 1);
  int okay = ffs_fwrite(&file, recovery_startup, size) == size;
  okay = ffs_fsync(&file) == 0 && okay;
  okay = ffs_fclose(&file) == 0 && okay;
  return okay;
}

int main(void) {
  const uint32_t sector = 2;
  uint24_t unlock;
  control_write_t restore_write;
  unsigned test_rc = 255, restore_rc = 255, verify_rc = 255;
  uint32_t before_crc = 0, pattern_crc = 0, observed_crc = 0, restored_crc = 0;

  puts("A10-RP04 physical raw-SD test starting.");
  if (!arm_recovery()) {
    puts("FAIL: could not disarm the one-shot startup; no raw write attempted.");
    return 19;
  }
  unlock = sd_getunlockcode();
  restore_write = fwbug008_control_write();
  if (!unlock || !restore_write || sd_init(unlock) != 0) {
    puts("FAIL: raw-SD initialization/control lookup failed; no write attempted.");
    return 19;
  }
  if (sd_readblocks(0, mbr, 1) != 0 || mbr[510] != 0x55 || mbr[511] != 0xaa ||
      mbr[450] == 0 ||
      le32(mbr + 454) <= sector) {
    puts("FAIL: sector 2 is not proven outside the first MBR partition; no write attempted.");
    return 19;
  }
  if (sd_readblocks(sector, before, 1) != 0) {
    puts("FAIL: sector preimage could not be retained; no write attempted.");
    return 19;
  }
  for (unsigned i = 0; i != sizeof pattern; ++i)
    pattern[i] = before[i] ^ (uint8_t)(0xa5U + i * 29U);
  before_crc = crc32(before, sizeof before);
  pattern_crc = crc32(pattern, sizeof pattern);

  test_rc = sd_writeblocks(sector, pattern, 1);
  unsigned read_rc = sd_readblocks(sector, observed, 1);
  observed_crc = crc32(observed, sizeof observed);

  /* Always attempt restoration through the independent C-dispatch control. */
  restore_rc = restore_write(sector, before, 1);
  verify_rc = sd_readblocks(sector, restored, 1);
  restored_crc = crc32(restored, sizeof restored);
  int restored_ok = restore_rc == 0 && verify_rc == 0 &&
                    memcmp(restored, before, sizeof before) == 0;
  int tested_ok = test_rc == 0 && read_rc == 0 &&
                  memcmp(observed, pattern, sizeof pattern) == 0;
  const char *status = tested_ok && restored_ok ? "pass" :
                       restored_ok ? "test-failure" : "restore-failure";
  int evidence_rc = save_result(status, test_rc, restore_rc, verify_rc,
                                before_crc, pattern_crc, observed_crc,
                                restored_crc);
  if (!restored_ok) {
    puts("FAIL: sector restoration was not verified. Do not continue tests.");
    return 20;
  }
  if (!tested_ok || evidence_rc) {
    puts("FAIL: repaired API did not write the expected bytes.");
    return 19;
  }
  puts("PASS: repaired API wrote sector 2 and the original sector was restored.");
  return 0;
}
