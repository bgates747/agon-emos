# EMOS v1 candidate qualification

This report separates automated candidate evidence from the mandatory human
emulator and later physical-hardware gates. Machine-readable measurements are
in `evidence/emos-v1-qualification.json`.

## Automated result

The current maintained-source snapshot prepares, converts, compiles, links,
and verifies as a 113,053-byte MOS image. The host suite, toolchain proof,
sixteen established C units plus `src/emos.c`, all fifteen assembly units,
restricted runtime, linker rejection cases, resident EMOS ABI, fixed VDU
dispatcher, deterministic integration media, target MOS API/formatter probe,
headless boot, directory-backed hostfs, bounded shell parity, linked VDP
regressions, and the complete eleven-command EMOS target-runtime transcript
pass. That runtime gate proves discovery, both provider classes and service
namespaces, fake Dual state, final Legacy recovery, and no media mutation.

Shell parity treats one change as intentional: the candidate HELP inventory
contains `EMOS` exactly once. After canonicalizing that addition and line
wrapping, all pre-existing commands, process-local variables, RTC output,
credits, curated directory traversal and file bytes, and error paths compare
equal to the official v3.0.2 image.

The official-image comparison generated a fresh report with 468 exact-name
symbol pairs, 40 matched module ranges, 51 complete assembly slices, and 27
medium-priority review entries. The entries are the known different-compiler
classes plus explained maintained-source changes. It intentionally fails the
reviewed-evidence check because this EMOS candidate has not replaced the
previous human-reviewed record; `binary-compare-record` was not run.

## Resource budget

The 128 KiB flash window retains 18,019 bytes. Internal RAM uses 5,959 static
bytes, reserves 2,048 stack bytes, and leaves 8,377 bytes between `__heapbot`
and `__heap_limit`. The external module area remains a separate fixed 32 KiB at
`0x0B0000`; payload entry is `0x0B0080`.

The resident EMOS C object contributes 9,831 text bytes, 397 read-only bytes,
no initialized data, and 3,167 BSS bytes before whole-image linking. Shipped
containers are 161 bytes for `hello` and 575 bytes each for `core.echo` and the
bounded Extender-shaped `edu.probe` fake.

## Evidence boundary

Headless Fab execution is automated functional evidence, not interactive
display approval, physical UART/EDP evidence, timing evidence, or hardware
qualification. The stock Platform VDP is used; this candidate does not build a
bespoke native VDP module. The profile-local `fab-agon-emulator` is now the
mandatory generated wrapper, with the raw upstream executable linked separately
as `fab-agon-emulator.bin`; direct wrapper help resolves the canonical local
SDL3 library path. The Author cold-booted from that exact entry after copying
`emos.commands` to `autoexec.txt`; the resulting screen shows the complete
clean transcript without diagnostics or retries and the final Legacy state at
mode generation 2 with registry generation 1. The Author explicitly approved
the emulator-coupled implementation and evidence milestones for commit and
push after reviewing that run.
The reviewed valid media is already staged in the isolated profile through the
fail-closed `make stage-emos-media` path; staging is idempotent and refuses to
overwrite divergent or unreviewed profile state.

## Human checkpoint

The completed human run used the canonical profile-local command:

```bash
cd emulator
./fab-agon-emulator
```

The Author copied the eleven lines in `emulator/sdcard/emos.commands` into the
profile's `autoexec.txt` and cold-booted the emulator. The reviewed outcomes
are: the Platform and MOS banners render normally; discovery reports
three providers; status begins in Legacy with onboard VDU route `0`, inactive
EDU, and the unavailable adapter; `HELLO` prints `Hello`; the two calls echo
`deterministic echo` and `separate edu result`; fake Dual reports route `0`,
active EDU, and adapter `fake`; and the final transition returns cleanly to
Legacy at mode generation 2 after the fake is disabled. The transcript contains
no diagnostics, retries, or unexpected output; the registry correctly remains
at generation 1. The runtime and commit portions of this checkpoint are
satisfied. Preserve the current profile `autoexec.txt` as Author-owned test
state.

The Author approved the commit checkpoint on 2026-08-24. The originally
reviewed maintained-source milestone was `agon-mos` commit `a8dc891`. Its exact
source tree is preserved in canonical `agon-emos` commit `0e1541c`; the
repository migration and reproduced artifact identity are recorded in
`docs/repository-migration.md`. The launcher-guidance milestone remains
`agon-dev-env` commit `a730cd7`.
