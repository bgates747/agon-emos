# Eirene controlled performance fixture

Original C/AgonDev application, with small timer and syscall assembly bridges.
The measurement contract is [performance.md](../../docs/tasks/TM-001/performance.md).
The minimal sender's [calling contract](../../docs/tasks/TM-001/contract.md)
applies; this app makes no changes to MOS, RST18, startup or VDP firmware.

From this repository, on Oryx, freeze a new snapshot:

```
python3 experiments/uart0-performance/prepare.py build/tm001/NEW
make -C build/tm001/NEW/fixture AGONDEV_TOOLCHAIN=/path/to/agondev
```

The generator shares the existing public VDP comparator source, pins the exact
baseline/scalar003/otirx003 firmware SHA256s, generates installed-ROM prefix
CRCs, and derives the app build ID from sources and firmware pins. Never reuse
a snapshot name: original failed attempts are evidence, not files to overwrite.

Use `load test.bin` then `run . 0` for pristine Eirene ordinary output, `run . 1`
for the scalar control, or `run . 2` for OTIRX. An installed-ROM mismatch refuses
the run before experimental syscalls. No firmware is flashed by this program.
Three repeats alternate bulk/paced order, with receiver-byte checks after each.
Paced means256payload-byte chunks averaging44100payload B/s, not44100single-byte
interrupts per second or an audio underrun/latency qualification. Reports are
sequential; do not clear next.txt/runs.log when preserving prior runs.

Stage STOPPED profiles using the shared qualified-runtime recipe:

```
python3 experiments/uart0-minimal/stage_flexe.py COHORT --suite performance --fixture NEW
```

Reserve/check out the shared Oryx lease before executing. The common runner
asserts ownership before every frontend, fails at the first failed cell, caps
each cell at300seconds and requires continual UART progress. After completion,
preserve raw evidence, stop only owned processes and release with fresh idle
evidence; notify the next recorded chat immediately if anyone is waiting.

```
UART0_ORYX_LEASE=TOKEN python3 experiments/uart0-minimal/run_flexe.py COHORT
python3 experiments/uart0-performance/analyze.py build/tm001/COHORT
```

Analysis independently checks xorshift work checksums, two timer counters,
submission/callback counts and all six emitted payload/comparator sequences.
Emulator ticks are fixture diagnostics, never physical speedup claims.
Console8 measurements decide the comparison. Work counts mean useful foreground
capacity, not directly measured CPU cycles; compare matched observation windows
and report missed deadlines alongside gains.

`package.py build/tm001/NEW build/tm001/APPLIANCE FULL_FIXTURE_COMMIT` prepares a fresh manual
appliance on Oryx with matching images, stock restoration and concise readme.txt.
It does not access or write an SD card. Physical flashing/execution remain
manual Author actions. Firmware and third-party licenses/credits are retained.
