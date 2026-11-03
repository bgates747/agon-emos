# Minimal Eirene UART0 validation

This finite fixture invokes Eirene's new experimental42h/43h syscalls. It is
not the old UTX1 application and does not measure physical performance.
Calling contract: ../../docs/tasks/TM-001/contract.md.

Build Eirene using its existing AgonDev Makefile on Oryx. The scalar control
changes only `UART0_ASYNC_SCALAR equ 0` to1 in a separate source snapshot. Keep
pristine upstream, scalar and OTIRX outputs in different directories. Preserve
matching mos.bin/mos.map pairs and tool/source hashes. No startup source,
RST18 or ordinary serial code is changed; the experimental label changes only
the already-printed VERSION_VARIANT string.

The original app is C/AgonDev with a small register-ABI bridge. Generate a fresh
fixture with `python3 experiments/uart0-minimal/prepare.py build/tm001/NEW`.
Run make in that snapshot with Oryx's configured AgonDev path. The packet
comparator is generated from source here and was checked byte-for-byte against
the previous qualified comparator, not fetched from a private build directory.

`RUN . 0` selects ordinary output; `RUN . 1` selects the async contract tests.
Each run clears the screen, reserves a fresh sequential Rnnnn.log and advances
next.txt. Allow up to two minutes and report the last progress line on a stall.
The app verifies the comparator first detects a wrong byte and accepts a right
one, then checks8192receiverpayloadbytes. Async cases additionally check lengths
1/15/16/17/255/256/257/1024, a NULL callback, rejected zero lengths and masked
context, second-write BUSY, C callback/source reuse and an early GP reply during
an8452-byte transmission. A temporary input-only keyboard hook is installed;
key traffic itself is not injected by this fixture. Fresh boot must have no
pre-existing user keyboard hook; the fixture restores the known zero state.

Do not return/free an accepted source after a timeout. The fixture retains its
application code/storage if it cannot finish. This is finite test behavior,
not additional MOS recovery machinery. The helper retries initial BUSY while
prior normal output reaches TEMT; it never retries an accepted source.

Oryx-only `stage_flexe.py NAME` stages six STOPPED guarded private profiles from
explicit frozen inputs (pristine/scalar003/otirx003 x JIT/interpreter). It currently
uses the maintained7e7d/086 runtime and matched receiver-fixed VDP201a. It is an
owner-local staging recipe; adapt paths/qualified runtime as an explicit cohort,
not by changing fleet defaults. The canonical setup tool generates/names the
wrapper; retain host-library, MOS/map, backend and runtime identity guards.

`run_flexe.py NAME` EXECUTES profiles. It requires UART0_ORYX_LEASE, asserts the
shared Pi checkout immediately before every emulator process, caps each cell
at180seconds, preserves raw output and stops at the first failure. Follow the
shared FIFO checkout/release and notify-next procedure; source/build permission
is not an emulator lease. Only owned processes are stopped. `analyze.py` checks
all emitted payload/comparator bytes and all boundary no-op bytes independently
of the app's PASS line, plus backend faults and UART drops/overruns.

First launch cohort flexe001 stopped before CPU execution: staging had omitted
the generated wrapper's executable bit. The fix and new flexe002 profiles passed
all six cells. Full records: ../../docs/tasks/TM-001/functional-r02.md.
Physical CTS margin, forced CTS edge races, keyboard traffic, return/crash misuse,
long-running apps and performance are outside this finite proof. No SD card or
physical firmware was changed. The next step is the paced/bulk performance
fixture, followed by a manually flashed Console8 comparison.
