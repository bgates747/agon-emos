# Smaller UART0 trial

TRIM Trial1 keeps background, interrupt-driven OTIRX sending and stock-style
MOS register saves. It removes broad context/foreign-owner checks and moves
extra callback saves into a fixed-target app wrapper. Published RC1 is unchanged.
Read [the caller contract](contract.md) before using the trial.

On Oryx, from a clean source-pinned checkout on codex/uart0-async:

```
AGONDEV_TOOLCHAIN=/absolute/path/to/agondev python3 experiments/uart0-trim/prepare.py build/tm001/tr001-001
```

The script archives HEAD, builds controlled branding/checks/wrappers/trim steps,
repeats the final clean build, checks unchanged source/object inputs, and builds
matching functional/example and PERF2 apps. It records SHA256, ROM CRC, compiler
identities, text/BSS/image sizes and identical compiled measurement objects.
No input build is overwritten. Run from the prescribed Oryx build checkout;
existing frozen RC1 and official stock MOS inputs must be present in build/.

Functional `test` checks borrowed-buffer boundaries, BUSY/NULL/zero-length,
callback/source reuse, concurrent RX, ordinary output and finish/re-entry.
It does not invoke the now-forbidden masked-context calls. `example` shows
foreground work with a fixed-target C callback wrapper. Both guard exact ROM.

The no-argument `stock`, `rc1` and `trim` PERF2 programs each cover bulk-only,
paced transmission plus calculation, and bulk plus calculation. Each has three
rotated repeats with8192 payload bytes and full receiver comparison per trial.
Only configuration and the required ABI bridge differ; the C measurement core
and timer object must match byte for byte. Trial wrapper overhead is included.
These new matched apps share external callback linkage. Do not silently merge
older firmware/app combinations into the new data set.

Initial isolated assembly measurements: RC1 476/11 text/BSS bytes; removed checks
402/11; simpler wrappers/session315/9; app-side callback saves286/9. The app gate
is counted separately, not claimed as vanished code. Whole-image and final app
sizes are recorded by preparation and qualification. No speedup is assumed.

Use actual-firmware Flexe only under the canonical Oryx lease. Model runs verify
function, not physical performance. Final Console8 comparison uses stock
Bistromathics2.16.0, with manual flashing and plain app commands in a concise
/test/mos-uart0 appliance. Keep history in project archives and continue
sequential report numbering. There is no new release/tag in this experiment.
