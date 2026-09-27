# REMOTE-005 — Provisional MOSlet SD listener admission

**Bounded M01–M03 scope complete.** The September 21 provisional result below
was superseded as the current installed combination by v0.1.19 and `/emos`
dispatch; see [AUDIT-008](AUDIT-008.md) and [EMOS utilities](../emos-utilities.md).
The historical 16-byte ROM margin is not the current budget. Wider user-facing
SD work remains in Extender REMOTE-005; broader EMOS acceptance stays with
AUDIT-008. This closure does not waive those remaining gates.

Author authorizes provisional firmware correction and bounded physical MOSlet
transfer checks. Existing listener fits32KiB; current gateway excludes its RAM.
Cross-component task: agon-extender docs/tasks/REMOTE-005.md.

M01 [x] Admit request/input/output wholly in caller RAM through B7FFF for resident
ext.sdlink only. Preserve other providers' module-space prohibition and MOS RAM.
M02 [x] Boundary tests and wrapper build/link checks, provisional EMOSv0.1.18.
M03 [x] Preserve actual running ROM/startup, deploy, verify ROM readback and test
MOSlet small upload/download, application sentinel, return/re-entry. No new protocol.

All bounded checks pass; full ROM readback verified, 1 KiB transfer and 4 KiB
application sentinel preserved, return/re-entry and self-write rejection pass.
Evidence: agon-extender docs/tasks/REMOTE-005/MOSLET-RESULTS.json. Provisionally
installed; only 16 ROM bytes remain. P4, VDP and autoexec unchanged.

## Commit closeout — 2026-09-23

Author authorized committing/pushing retained work. The focused sdlink host test
passes again with address/undefined-behavior sanitizers. The broader
`tests/test_emos_port.py` run reports eight failures (including subtests): its
prepared snapshot lacks EMOS sources, generated-assembly inventory lacks three
EMOS assembly units, and ZDS-project path assertions fail. Those checks do not
provide a clean whole-port result in this environment. No build artifacts,
ZDS project or generic port tooling changed during this closeout; retained
September 21 wrapper-build and physical results above remain separately scoped.

## A04 admission continuation — 2026-09-27


The cross-component authority is Extender's REMOTE-005. This file owns only
EMOS implementation and validation. Current work is developmental and must not
be deployed as production.

R05-A04 [ ] Private top-level editor wake, bounded control exchange and checked
finite `/emos/sdjob.bin` dispatch implemented. Full build/linked checks, 112 host
tests and five UART-peer emulator scenarios pass. Await Author emulator review
before commit. No actual automatic file-transfer service yet.

R05-A05 [ ] Application SDK and finite service integration follow the Extender
contract; not started by this tranche.

## Source/contract and checks

Use the sibling Extender documents `docs/tasks/REMOTE-005/ADMISSION-CONTRACT.md`
and `A04-RESULTS/README.md` for protocol and retained cross-component evidence.
The new module is src/emos_admission.c; public editor API remains unchanged.
Official references were reviewed at MOS v3.0.2 and docs API 0x09 as recorded
in Extender's ADMISSION-REVIEW.md. Tests/test_emos_admission.py executes the
maintained C state machine; scripts/admission_peer.py uses a retained UART-peer
Fab runtime and the actual target image. The admission-probe is test-only.

## Limits

Only complete 48-byte admission controls are implemented here. No READY or
successful file-completion report is emitted. A utility returning zero without
that protocol still reports unavailable. A future utility must not equate a
normal MOSlet return with confirmed commit. Stalled/missing peers fail closed;
manual sdserve retains its existing lifecycle and Legacy restriction. Do not
remove the ExCom guard until paired parser/data transport work is validated.

### A04 physical qualification — 2026-09-27

Author released the bench. Corrected build
`agon-emos-v0.1.20-b2026-09-27-21-03-57Z` (source `01d07a9`) passed complete ROM
readback and bounded physical admission checks using Extender's temporary r58
peer. Initial hardware run exposed stale `job_class` after failed utility load;
reset now clears it and a regression test covers subsequent NO_WORK polling.
All 112 host tests pass. Physical checks cover missing/corrupt/raced offers,
4096-byte application sentinel, nested loader exclusion, ordinary public editor,
durable result, manual-listener isolation, and Legacy/ExCom routing restoration.
Normal P4 r57 restored; temporary sdjob probe removed from the dispatch path;
startup unchanged. This is a development candidate, not a production promotion
or a working automatic file-transfer engine. Detailed evidence belongs to
Extender `docs/tasks/REMOTE-005/A04-HARDWARE.md` and its results JSON.

### A05 linked application helper — development

Implemented application-owned transport lease plus caller-linked checked transfer
helper under `lib/sdapp`; no second utility load, file engine remains outside ROM.
Extender owns `docs/tasks/REMOTE-005/A05-CONTRACT.md` and qualification results.
Host tests cover both directions/faults; target UART-peer send proves caller return
and 4096-byte sentinel preservation. EMOS image 127699 bytes (+207 over A04),
3373 bytes below 128 KiB. Receive durability cannot be qualified by the retained
emulator's unsupported filesystem-sync interception. Physical P4 staging and ExCom
remain downstream. Source frozen at `7cd480e`; v0.1.21 installed and full ROM
readback verified. Both directions pass with the diagnostic RAM-only P4 peer;
4096-byte sentinel, external-request rejection and subsequent manual listener pass.
The test harness must wait for a fresh idle POLL after transfer completion before
typing the next command. Earlier premature collection showed Invalid executable;
no speculative firmware fix was added. Normal P4 restored after qualification.
Production unchanged; emulator-specific driver remains pending human review.

## A07 directory extension — local development only

The EMOSlet now implements capability 0x20: MKDIR 12, REMOVE 13 (execute or
preflight), and fragmented no-overwrite MOVE 14. Extender owns the shared wire
contract and recursive host orchestration. The engine uses public MOS FatFS
APIs; no resident ROM source changed. Root, journal and maintained utility
protection applies before mutation; incomplete move fragments never rename.

129 EMOS tests pass with the A05 prepared EMOS baseline; the engine subset has
42 checks across normal/fast variants. AgonDev MOSlet compile: 21981 bytes,
8216 bytes remaining heap/stack region. No hardware or emulator qualification
was performed. Installed v0.2.0 and production selection remain unchanged.
Extender docs/tasks/REMOTE-005/A07-RESULTS.md records cross-component evidence.
