# Mainboard SD foreground service

The listener uses resident `ext.sdlink` and matching Extender P4 firmware.
The recorded current installation is the sdserve v0.2.0 EMOSlet at
`/emos/sdserve.bin`, launched through EMOS v0.1.19:

```text
EMOS sdserve /
```

Add `--fast` before or after `/` for opt-in fast uploads. The service requires
Legacy mode and healthy Extender keyboard admission **before launch**. A remote
agent cannot enable its own unavailable keyboard path by typing through it.
Use an operator or prepared startup to establish `EMOS KEYINPUT extender`, then
start at a verified prompt. No custom onboard VDP or graphics mode change is
required. An EMOSlet runs in the foreground, not alongside a game.

The root is an absolute existing directory. `/` permits the whole card;
`/extender/sdtest` remains the no-argument default and must be provisioned first.
The endpoint is unauthenticated, for a trusted LAN. Change root or checked/fast
mode by stopping and restarting the listener. Escape returns to its caller and
preserves unfinished stages; if the caller is a batch, its next command can run.
Network EXIT requires no active transfer. Neither operation proves an idle CLI.

## Maintained authorities

| Subject | Authority |
|---|---|
| Invocation, host client, sessions, backup cleanup and recovery | [Extender SD operating guide](https://github.com/bgates747/agon-extender/blob/main/docs/mainboard-sd.md) |
| Packet layouts and ownership | [Extender SD wire contract](https://github.com/bgates747/agon-extender/blob/main/docs/protocols/mainboard-sd.md) |
| Prefixed dispatch, executable bounds and caller restrictions | [EMOS utilities](../../docs/emos-utilities.md) |
| SD file placement and historical path changes | [SD layout](https://github.com/bgates747/agon-extender/blob/main/docs/sd-layout.md) |
| Latest scoped physical installation | [AUDIT-008 hardware receipt](https://github.com/bgates747/agon-extender/blob/main/docs/tasks/AUDIT-008/HARDWARE.md) |

`src/sd_wire.h` is a reviewed copy of the Extender-owned codec; keep both copies
synchronized. Compilation does not require a sibling checkout. EMOS owns UART
admission and bounded mailboxes; the listener owns foreground MOS/FatFS calls.
It does not install callbacks, take over UART, launch games or reset hardware.
Do not replace running or held-open files, including an EXEC/autoexec batch
whose child is this listener. `FF_FS_LOCK=0` is not a global open-file monitor.

## Build layouts

With AgonDev on PATH, from this repository root:

```sh
# Ordinary application fallback (LOAD/RUN replaces application RAM).
make -C projects/sdserve clean
make -C projects/sdserve

# MOSlet for /emos/sdserve.bin (32 KiB region at B0000).
make -C projects/sdserve clean
make -C projects/sdserve RAM_START=0xB0000 RAM_SIZE=0x8000
```

Both produce `projects/sdserve/bin/sdserve.bin`. Clean when changing layouts;
do not interchange the binaries. `AGONDEV_TOOLCHAIN=/absolute/path/to/agondev`
can select the toolchain explicitly. Preserve the `sdserve.bin` basename: the
wire service rejects writes to that basename to protect its executing file.
A compile-only unversioned build is not a deployment candidate.

`scripts/prepare_sdserve.py --output build/sdserve-candidate --toolchain ...`
prepares an identified **ordinary application** bundle, with source/compiler
hashes. It does not select the MOSlet layout. The identified MOSlet workflow
and frozen validation are in Extender's
[fast-transfer tasklet](https://github.com/bgates747/agon-extender/blob/main/docs/tasks/REMOTE-005/FAST-TRANSFER.md).
The ordinary fallback remains `/extender/sdserve.bin`, using
`LOAD /extender/sdserve.bin` then `RUN . /`. The installed listener moved out of
`/mos`; bare `sdserve` is not its current installed invocation.

## Checked versus fast uploads

Normal mode checks sync/close and rereads staged and activated bytes for
length/CRC. The host additionally downloads both copies for comparison. Fast
mode requires `--fast` on the listener and on the host client's `put` command;
a mismatch is rejected before BEGIN. HELLO bit 0x10 advertises fast mode.
Fast FINISH echoes the declared CRC, not a measured one. Packet CRCs, checked
byte counts, sync/close, staging, backups, replay and recovery remain, but fast
success does not verify stored contents. Both modes retain target-adjacent
`.p17part`, `.p17meta` and `.p17bak` files. FAT rename is not power-failure atomic;
follow the operating guide rather than manually deleting an uncertain journal.

## Validation scope

Run from the repository root:

```sh
.venv/bin/python -m unittest discover -s tests -p test_sdserve.py -v
.venv/bin/python -m unittest discover -s tests -p test_emos_sdlink.py -v
```

Injected POSIX failures exercise the real service engine, not physical FAT.
Extender's raw-FAT emulator runs exercise compiled eZ80/EMOS/FatFS with the P4
queue and HTTP client. Directory-backed emulator SD has documented create-new
and sync differences; use the raw-FAT fixture for those semantics.

The original EMOS v0.1.14 / listener v0.1.0 physical check is retained in the
[September 13 log](../../research/devlog/2026-09-13.md). On September 24, fast
v0.2.0 at its then `/mos` location passed eight bounded transfers and measured
5.00× end-to-end throughput for two 8192-byte uploads per mode. Later v0.1.19
checks passed `/emos` dispatch, normal/fast transfer, reentry and an application
sentinel. Those receipts do not qualify every rebuilt binary or broader gameplay.
