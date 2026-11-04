# Preparing Eirene UART0 RC1

This recipe builds the selected minimal OTIRX firmware with its RC banner,
without changing sender/startup code. It retains Eirene's AgonDev compiler.
Commit source first. On Oryx, using the selected pinned toolchain:

```sh
AGONDEV_TOOLCHAIN=/absolute/path/to/agondev python3 experiments/uart0-release/prepare.py build/tm001/rc1-001
python3 experiments/uart0-release/stage.py rc1-001 rc1-flexe001
```

prepare.py archives the exact source commit, builds firmware twice, requires
identical MOS bytes, compares sources/objects with the previously qualified
OTIRX build, then builds a no-argument `test` and `example` matched to its ROM
CRC. Compiler/library hashes, logs and generated source remain in the output.
The frozen app directories build independently with their included Makefiles.

The focused fixture reuses the earlier functional checks: ordinary RST18 before
and after async, source boundaries, NULL callback, masked/zero-length rejection,
second-submission BUSY, source reuse, RX during TX and every8192payload byte.
It writes sequential Rnnnn.log and START/PASS journal entries. Type `test` twice
from /test/mos-uart0 after flashing the candidate and confirming its cold-boot
RC banner. Each invocation must return RC1 PASS and the ordinary MOS prompt.
Then type `example` and require RC1 EXAMPLE PASS. No switches or LOAD/RUN.

stage.py creates two stopped guarded Flexe profiles, one per engine, using the
pinned receiver-fixed VDP/runtime. Join the canonical shared FIFO, check out with
fresh whole-host idle evidence and assert immediately before each process:

```sh
UART0_ORYX_LEASE=TOKEN python3 experiments/uart0-minimal/run_flexe.py rc1-flexe001
python3 experiments/uart0-release/analyze.py rc1-flexe001
```

The runner caps each cell, preserves evidence and stops at first failure.
Release with fresh idle evidence and notify the next agent. The independent
analyzer checks the RC banner, both reports, payload/comparator commands,
boundary bytes, example checksum and backend/UART faults. It makes no hardware
or performance claim. The focused manual Console8 check remains separate.

No extra sender feature or MOS recovery framework is added for release.
See ../../docs/tasks/RC-001/README.md for promotion state and exact-image gate.

For RC1 the Author waived branded-image emulator checks in favor of the final
Console8 check. Build/package without a model claim using:

```sh
python3 experiments/uart0-release/package.py rc1-003 - rc1-package001 --hardware-only
```

The prepared stopped profiles were not executed; their reservation was cancelled.
