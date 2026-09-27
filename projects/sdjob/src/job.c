/* Finite, claimed external service. Filesystem work stays in this foreground
 * MOSlet; resident code only supplies the binding and closes the grant.
 * Checked sdserve engine remains authoritative for FAT writes/recovery.
 * STATUS is cooperative, including during engine digest loops. Cancellation
 * never interrupts an activation half way: finish that engine call, preserve
 * recovery state, then close. No automatic retry of any file operation.
 */
#include "job.h"
#include "../../../lib/sdapp/sdapp.h"
#include "../../sdserve/src/service.h"
#include <agon/mos.h>
#include <string.h>
static uint8_t binding[36], descriptor[248], tx[240], rx[240], packet[240],
    reply[240], pending[240];
static char source[121], destination[121], transfer_path[121];
static unsigned descriptor_n, pending_n, failed, ending, in_control, live,
    mutation, stage_open;
static uint32_t sequence, session, file_session, last_status, start_time;
static int valid(const uint8_t *p, unsigned n) {
  uint32_t c;
  if (n < 20 || n > 240 || p[0] != 'S' || p[1] != 'D' || p[2] != 1 ||
      sd_u16(p + 14) != n - 20)
    return 0;
  c = sd_crc_update(UINT32_C(0xffffffff), p, 16);
  return (sd_crc_update(c, p + 20, n - 20) ^ UINT32_C(0xffffffff)) ==
         sd_u32(p + 16);
}
static int control(unsigned op, unsigned result, const uint8_t *extra,
                   unsigned n, unsigned *received) {
  unsigned got, spins = 1000000;
  uint32_t at = sdapp_clock();
  int ok = 0;
  if (in_control || sequence >= UINT32_MAX - 1 || n > 192)
    return 0;
  in_control = 1;
  sd_header(tx, 4, session, ++sequence, op, result, 28 + n);
  memcpy(tx + 20, binding, 28);
  if (n)
    memcpy(tx + 48, extra, n);
  sd_seal(tx);
  if (sdapp_link(2, tx, 48 + n, 0, 0, &got))
    goto done;
  do {
    if (sdjob_cancelled())
      failed = 1;
    if (sdapp_link(1, 0, 0, rx, sizeof rx, &got))
      goto done;
    if (got) {
      if (!valid(rx, got))
        goto done;
      if (rx[3] == SD_REQUEST && live && sd_u32(rx + 4) == file_session) {
        if (pending_n)
          goto done;
        memcpy(pending, rx, got);
        pending_n = got;
        continue;
      }
      if (got < 48 || rx[3] != 5 || memcmp(rx + 4, tx + 4, 9) ||
          memcmp(rx + 20, binding, 28))
        goto done;
      *received = got;
      ok = 1;
      goto done;
    }
  } while (--spins && (uint32_t)(sdapp_clock() - at) < 24);
done:
  in_control = 0;
  return ok;
}
void service_progress(void) {
  unsigned n;
  uint32_t now = sdapp_clock();
  if (sdjob_cancelled() || (uint32_t)(now - start_time) >= 72000)
    failed = 1;
  if (!live || in_control || failed || (uint32_t)(now - last_status) < 12)
    return;
  last_status = now;
  if (!control(7, 0, 0, 0, &n) || n != 49 || rx[13] > 0 || rx[48] > 1)
    failed = 1;
  else if (rx[48])
    ending = 1;
}
static int path_ok(const char *p) {
  unsigned i, start = 1;
  if (p[0] != '/')
    return 0;
  if (!p[1])
    return 1;
  for (i = 1; i <= 120; i++) {
    unsigned char c = p[i];
    if (c && (c < 32 || c > 126 || strchr(":\\*?\"<>|%", c)))
      return 0;
    if (c == '/' || !c) {
      unsigned n = i - start;
      if (!n || (n == 1 && p[start] == '.') ||
          (n == 2 && p[start] == '.' && p[start + 1] == '.') ||
          p[i - 1] == '.' || p[i - 1] == ' ')
        return 0;
      start = i + 1;
    }
    if (!c)
      return 1;
  }
  return 0;
}
static unsigned fold(unsigned c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int equal(const char *a, const char *b) {
  while (*a && *b && fold(*a) == fold(*b)) {
    a++;
    b++;
  }
  return !*a && !*b;
}
static int below(const char *path, const char *root, int recursive) {
  if (!*root)
    return 0;
  if (equal(path, root))
    return 1;
  if (!recursive)
    return 0;
  if (!strcmp(root, "/"))
    return 1;
  while (*root && *path && fold(*root) == fold(*path)) {
    root++;
    path++;
  }
  return !*root && *path == '/';
}
static int reserved(const char *p) {
  char lower[121];
  unsigned i;
  for (i = 0; p[i]; i++)
    lower[i] = fold(p[i]);
  lower[i] = 0;
  return strstr(lower, ".p17part") || strstr(lower, ".p17meta") ||
         strstr(lower, ".p17bak");
}
static int descriptor_ok(void) {
  unsigned a = sd_u16(descriptor + 2), b = sd_u16(descriptor + 4),
           c = descriptor[0];
  if (descriptor_n < 9 || c < 1 || c > 8 || c != binding[25] ||
      descriptor[1] > 3 || sd_u16(descriptor + 6) || a > 120 || b > 120 ||
      8 + a + b != descriptor_n)
    return 0;
  if (memchr(descriptor + 8, 0, a + b))
    return 0;
  memcpy(source, descriptor + 8, a);
  source[a] = 0;
  memcpy(destination, descriptor + 8 + a, b);
  destination[b] = 0;
  if ((a && (!path_ok(source) || reserved(source))) ||
      (b && (!path_ok(destination) || reserved(destination))))
    return 0;
  if (c == 6 || c == 7) {
    if (!a || !b || below(source, destination, 1) ||
        below(destination, source, 1))
      return 0;
  } else if (c == 3 || c == 5) {
    if (a || !b)
      return 0;
  } else if (!a || b)
    return 0;
  if ((b && equal(destination, "/")) ||
      ((c == 6 || c == 8) && equal(source, "/")))
    return 0;
  return 1;
}
static int get_descriptor(void) {
  unsigned at = 0, n, total = 0;
  uint32_t crc = 0;
  uint8_t offset[2];
  do {
    sd_put16(offset, at);
    if (!control(6, 0, offset, 2, &n) || rx[13] || n <= 56 || n > 240)
      return 0;
    if (!at) {
      total = sd_u16(rx + 48);
      crc = sd_u32(rx + 52);
      if (total < 9 || total > 248)
        return 0;
    }
    if (sd_u16(rx + 48) != total || sd_u16(rx + 50) != at ||
        sd_u32(rx + 52) != crc || n - 56 > total - at)
      return 0;
    memcpy(descriptor + at, rx + 56, n - 56);
    at += n - 56;
  } while (at < total);
  descriptor_n = total;
  return sd_crc(descriptor, total) == crc && descriptor_ok();
}
static int get_path(const uint8_t *p, unsigned n, char *out) {
  if (!n || p[0] > 120 || p[0] + 1U != n || memchr(p + 1, 0, p[0]))
    return 0;
  memcpy(out, p + 1, p[0]);
  out[p[0]] = 0;
  return path_ok(out);
}
/* Reject wrong operation/path before handing a record to the file engine.
 * READ of the current owned .p17part is the one journal visibility exception.
 * The engine independently validates transfer IDs, offsets, CRC and sequences.
 */
static int allowed(unsigned op, const uint8_t *p, unsigned n) {
  unsigned c = descriptor[0], recursive = (descriptor[1] & 2) != 0;
  char path[121];
  int src, dst;
  unsigned offset = 0;
  if (op == SD_HELLO)
    return !n;
  if (op == SD_WRITE || op == SD_FINISH || op == SD_ACTIVATE || op == SD_CANCEL)
    return (c == 3 || c == 7) && transfer_path[0];
  if (op == SD_MOVE) {
    unsigned at, count;
    if (c != 6 || n < 9 || equal(source, "/emos/sdjob.bin") ||
        equal(destination, "/emos/sdjob.bin"))
      return 0;
    at = sd_u16(p + 2);
    count = n - 8;
    return sd_u16(p) == descriptor_n && at <= descriptor_n &&
           count <= descriptor_n - at &&
           sd_u32(p + 4) == sd_crc(descriptor, descriptor_n) &&
           !memcmp(p + 8, descriptor + at, count);
  }
  if (op == SD_READ)
    offset = 6;
  else if (op == SD_LIST)
    offset = 4;
  else if (op == SD_BEGIN)
    offset = 12;
  else if (op == SD_REMOVE)
    offset = 1;
  else if (op != SD_STAT && op != SD_MKDIR)
    return 0;
  if (n <= offset || !get_path(p + offset, n - offset, path))
    return 0;
  if (op == SD_READ && transfer_path[0]) {
    char stage[129];
    strcpy(stage, transfer_path);
    strcat(stage, ".p17part");
    if (equal(stage, path))
      return 1;
  }
  if (reserved(path))
    return 0;
  src = below(path, source, recursive && (c == 2 || c == 7 || c == 8));
  dst = below(path, destination, recursive && c == 7);
  if (op == SD_STAT)
    return src || dst;
  if (op == SD_LIST)
    return src && (c == 2 || c == 7 || c == 8);
  if (op == SD_READ)
    return (src && (c == 4 || c == 7)) || (dst && (c == 3 || c == 7));
  if ((op == SD_BEGIN || op == SD_MKDIR || op == SD_REMOVE) &&
      equal(path, "/emos/sdjob.bin"))
    return 0;
  if (op == SD_BEGIN)
    return dst && (c == 3 || c == 7);
  if (op == SD_MKDIR)
    return dst && (c == 5 || c == 7);
  if (op == SD_REMOVE)
    return src && c == 8;
  return 0;
}
int sdjob_run(void) {
  unsigned n, got, opened = 0, initialized = 0, spins = 1000000;
  uint32_t last_clock;
  uint8_t terminal[5], outcome;
  failed = ending = in_control = live = mutation = pending_n = stage_open = 0;
  transfer_path[0] = 0;
  if (sdapp_link(5, 0, 0, binding, sizeof binding, &n) || n != 36)
    return 35;
  opened = 1;
  session = sd_u32(binding + 28);
  sequence = sd_u32(binding + 32);
  file_session = sd_crc(binding, 28);
  if (!file_session)
    file_session = 1;
  start_time = last_status = last_clock = sdapp_clock();
  if (!session || binding[24] != 1 || binding[26] || binding[27] ||
      !get_descriptor() || failed)
    goto fail;
  if (!service_init_mode("/", file_session, 0))
    goto fail;
  initialized = 1;
  if (!control(4, 0, 0, 0, &n) || n != 48 || rx[13] || failed)
    goto fail;
  live = 1;
  while (!failed && !ending) {
    if (pending_n) {
      n = pending_n;
      memcpy(packet, pending, n);
      pending_n = 0;
    } else if (sdapp_link(1, 0, 0, packet, sizeof packet, &n))
      goto fail;
    if (n) {
      int ok;
      if (!sd_valid(packet, n) || packet[3] != SD_REQUEST || packet[13] ||
          sd_u32(packet + 4) != file_session || !sd_u32(packet + 8))
        goto fail;
      ok = allowed(packet[12], packet + 20, n - 20);
      if (ok && packet[12] == SD_BEGIN && !(descriptor[1] & 1)) {
        char path[121];
        FILINFO info;
        uint8_t status;
        if (!get_path(packet + 32, n - 32, path))
          goto fail;
        status = ffs_stat(&info, path);
        if (!status)
          ok = 0;
        else if (status != FR_NO_FILE && status != FR_NO_PATH)
          goto fail;
      }
      if (!ok)
        goto fail;
      got = service_request(packet, n, reply);
      if (!got)
        goto fail;
      if (!reply[13]) {
        if (packet[12] == SD_BEGIN) {
          (void)get_path(packet + 32, n - 32, transfer_path);
          stage_open = 1;
        }
        if (packet[12] == SD_ACTIVATE || packet[12] == SD_CANCEL)
          stage_open = 0;
        if (packet[12] == SD_ACTIVATE || packet[12] == SD_MKDIR ||
            packet[12] == SD_REMOVE || packet[12] == SD_MOVE)
          mutation = 1;
      }
      if (sdapp_link(2, reply, got, 0, 0, &got))
        goto fail;
      spins = 1000000;
    }
    service_progress();
    {
      uint32_t now = sdapp_clock();
      if (now != last_clock) {
        last_clock = now;
        spins = 1000000;
      } else if (!--spins)
        goto fail;
    }
  }
  goto finish;
fail:
  failed = 1;
finish:
  if (stage_open)
    failed = 1;
  if (initialized)
    service_stop();
  outcome = failed ? 3 : mutation ? 1 : 0;
  if (!control(9, failed ? 7 : 0, &outcome, 1, &n) || n != 48 || rx[13])
    failed = 1;
  live = 0;
  sd_put32(terminal, sequence);
  terminal[4] = failed ? 1 : 0;
  if (sdapp_link(6, terminal, 5, 0, 0, &got))
    failed = 1;
  if (opened && sdapp_link(3, 0, 0, 0, 0, &got))
    failed = 1;
  return failed ? 35 : 0;
}
