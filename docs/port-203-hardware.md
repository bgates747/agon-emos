# PORT-203 physical qualification procedure

This is the **historical provider-era procedure**, not a current deployment
recipe. Its fake modes, provider discovery and corrupt-module stages concern
the cancelled external `.emo` loader. Keep it with the original
[PORT-200 evidence](port-200-qualification.md); do not use those stages to
qualify today's resident services or foreground utilities. Current contracts
start at the [documentation index](README.md). A new physical qualification
requires an owner-reviewed procedure for its exact candidate and current
Extender recovery, SD placement and bench constraints.

This is an author-operated gate. Emulator success does not complete it.

## Preparation and rollback

1. Run `make MOS_WORKTREE=PATH FAB_ROOT=PATH qualify` and retain the complete
   log.
2. Record SHA-256 for the candidate
   `mos-agondev/projects/mos-port/bin/MOS.bin`, the VDP firmware, the
   recovery MOS image, and the test SD image. Copy the candidate and recovery
   image to separate, clearly labelled media.
3. Identify board model/revision, eZ80 clock, VDP build, storage type/capacity,
   UART peer/levels/baud, EDP carrier and power source. Disconnect unsupported
   peripherals.
4. Verify the board's documented recovery/flash method with the recovery image
   before installing the candidate. Abort if recovery is not repeatable.
5. Power down before firmware/media/cable changes. Keep the recovery medium
   physically separate. Never format or label the operator's only copy.

## Staged run

Run stages in order and stop on the first failure, unexpected reset, storage
corruption, electrical anomaly, or inability to recover:

1. `boot`: three cold boots; banner, prompt, keyboard editing, reset and
   recovery path; no hang or unexpected reset.
2. `storage`: disposable media only; create/write/sync/rename/read after cold
   boot/truncate/unlink, directory traversal, >16 MiB file size/EOF, and
   deliberate missing/read-only/full-media errors. Hash the media before and
   after and retain recovered test files.
3. `rtc`: set a known non-boundary value, cold boot, read it back within two
   seconds plus elapsed wall time, then restore the original value.
4. `uart-vdp`: exercise UART FIFO pressure, nonblocking empty read, framing/
   parity/overrun conditions where supported, sustained VDP packets, legal
   16-byte and rejected 17-byte packets, and measured handshake latency. Record
   logic-analyzer or peer logs; do not infer timing from Fab.
5. `applications`: run the agreed BASIC/editor/file-manager/MOSlet corpus and
   compare saved-output hashes with the official MOS baseline.
6. `emos`: run Legacy, fake Dual, provider discovery and calls; if supported
   EDP hardware is present, repeat with real discovery and adapter recovery.
7. `failure-recovery`: corrupt a disposable module, remove media during an
   idle-safe point, request unavailable modes, and prove the prior registry,
   Legacy route, and recovery firmware remain usable.

Acceptance requires every applicable stage to pass, every skipped cell to have
a reason, no unexplained reset/data change, and successful final rollback.
Capture raw logs/photos/analyzer files without editing them. Record each file's
SHA-256 in one JSON capture and validate it with:

```bash
python3 scripts/verify_hardware_capture.py \
  --firmware MOS_AGONDEV_ROOT/projects/mos-port/bin/MOS.bin CAPTURE.json
```

Only an author-approved capture produced by real supported hardware may close
QUAL-001 or support a hardware-qualified/replacement-release claim.
