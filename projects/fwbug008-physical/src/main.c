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
extern void fwbug008_get_unlock(uint24_t *destination);
extern uint8_t fwbug008_init(const uint24_t *unlock);
extern uint8_t fwbug008_rst_write(const void *request, uint8_t *buffer,
                                  uint16_t count);
extern control_write_t fwbug008_control_write(void);

static uint8_t mbr[512], before[512], pattern[512], observed[512], restored[512];
static const char recovery_startup[] =
    "SET KEYBOARD 1\r\n"
    "EMOS KEYINPUT extender\r\n"
    /* The P4 can retain ExCom across an eZ80 reset. VDU 22 selects geometry;
     * it does not return EMOS transport ownership to Legacy. */
    "EMOS LEGACY\r\n"
    "VDU 22 3\r\n"
    "EMOS sdserve --fast /\r\n";

/* Positive host handoff: RUN returning is not a remotely observable fixture
 * completion signal.  Start the result service here only after the fixture is
 * known not to have touched media, or after sector restoration was verified.
 * The host must never infer that a timeout makes an in-flight raw write safe
 * to interrupt with a reset. */
static int serve_result(int result) {
  char command[] = "EMOS sdserve --fast /";
  int service = mos_oscli(command, NULL, 0);
  if (service) printf("FAIL: result service returned %d.\r\n", service);
  return service ? service : result;
}

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
                       uint32_t restored_crc, const char *detail,
                       uint32_t first_partition_lba, unsigned write_attempted) {
  FIL file = {0};
  char record[384];
  int n = snprintf(record, sizeof record,
      "schema=1\ncase=a10-rp04-raw-sd-write\nstatus=%s\nsector=2\n"
      "test_rc=%u\nrestore_rc=%u\nrestore_verify_rc=%u\n"
      "before_crc32=%08lx\npattern_crc32=%08lx\n"
      "observed_crc32=%08lx\nrestored_crc32=%08lx\n"
      "detail=%s\nfirst_partition_lba=%lu\nwrite_attempted=%u\n",
      status, test_rc, restore_rc, verify_rc,
      (unsigned long)before_crc, (unsigned long)pattern_crc,
      (unsigned long)observed_crc, (unsigned long)restored_crc, detail,
      (unsigned long)first_partition_lba, write_attempted);
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
  uint8_t write_request[7];
  control_write_t restore_write;
  unsigned test_rc = 255, restore_rc = 255, verify_rc = 255;
  uint32_t before_crc = 0, pattern_crc = 0, observed_crc = 0, restored_crc = 0;
  uint32_t first_partition_lba = UINT32_MAX;

  puts("A10-RP04 physical raw-SD test starting.");
  if (!arm_recovery()) {
    (void)save_result("infrastructure-error", 255, 255, 255, 0, 0, 0, 0,
                      "recovery-arm-failed", first_partition_lba, 0);
    puts("FAIL: could not disarm the one-shot startup; no raw write attempted.");
    return serve_result(19);
  }
  fwbug008_get_unlock(&unlock);
  restore_write = fwbug008_control_write();
  unsigned init_rc = unlock ? fwbug008_init(&unlock) : 255;
  if (!unlock || !restore_write || init_rc != 0) {
    (void)save_result("infrastructure-error", init_rc, 255, 255, 0, 0, 0, 0,
                      !unlock ? "unlock-zero" :
                      !restore_write ? "control-lookup-zero" : "sd-init-failed",
                      first_partition_lba, 0);
    puts("FAIL: raw-SD initialization/control lookup failed; no write attempted.");
    return serve_result(19);
  }
  unsigned mbr_rc = sd_readblocks(0, mbr, 1);
  if (!mbr_rc) {
    for (unsigned entry = 0; entry != 4; ++entry) {
      const uint8_t *partition = mbr + 446 + entry * 16;
      uint32_t start = le32(partition + 8), count = le32(partition + 12);
      if (partition[4] && count && start < first_partition_lba)
        first_partition_lba = start;
    }
  }
  if (mbr_rc != 0 || mbr[510] != 0x55 || mbr[511] != 0xaa ||
      first_partition_lba <= sector || first_partition_lba == UINT32_MAX) {
    (void)save_result("infrastructure-error", mbr_rc, 255, 255, 0, 0, 0, 0,
                      mbr_rc ? "mbr-read-failed" : "unsafe-card-layout",
                      first_partition_lba, 0);
    puts("FAIL: sector 2 is not proven outside the first MBR partition; no write attempted.");
    return serve_result(19);
  }
  unsigned preimage_rc = sd_readblocks(sector, before, 1);
  if (preimage_rc != 0) {
    (void)save_result("infrastructure-error", preimage_rc, 255, 255, 0, 0, 0, 0,
                      "preimage-read-failed", first_partition_lba, 0);
    puts("FAIL: sector preimage could not be retained; no write attempted.");
    return serve_result(19);
  }
  for (unsigned i = 0; i != sizeof pattern; ++i)
    pattern[i] = before[i] ^ (uint8_t)(0xa5U + i * 29U);
  before_crc = crc32(before, sizeof before);
  pattern_crc = crc32(pattern, sizeof pattern);

  memcpy(write_request, &sector, sizeof sector);
  memcpy(write_request + sizeof sector, &unlock, sizeof unlock);
  test_rc = fwbug008_rst_write(write_request, pattern, 1);
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
                                restored_crc, "completed", first_partition_lba, 1);
  if (!restored_ok) {
    puts("FAIL: sector restoration was not verified. Do not continue tests.");
    return 20;
  }
  if (!tested_ok || evidence_rc) {
    puts("FAIL: repaired API did not write the expected bytes.");
    return serve_result(19);
  }
  puts("PASS: repaired API wrote sector 2 and the original sector was restored.");
  return serve_result(0);
}
