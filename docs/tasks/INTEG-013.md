# INTEG-013 — Owned mainboard SD-service transport gateway

Author released Extender PORT-017 on 2026-09-12; this is its EMOS component.
Highest EMOS priority until the SD capability is accepted. Wire contract:
agon-extender `docs/tasks/PORT-017/PROTOCOL.md`; this task owns maintained eZ80
transport code, the foreground `projects/sdserve` application and qualification.
P4/network ownership remains in agon-extender.

1. Add reserved `ext.sdlink` to the existing public gateway, preserving all
   existing APIs and ordinary VDU/input behavior. Validate request/output memory,
   Legacy mode, Extender keyboard ownership, operations and lengths.
2. Receive bounded private 8D service records through the existing UART1 parser.
   Keep one mailbox and never invoke filesystem work or game keyboard callbacks
   for these packets. No direct application UART/vector/register access.
3. Send private F6 envelopes only through the EMOS-owned writer. Preserve
   keyboard receive interrupts. Invalidate admission/mailbox on fault, application
   entry/exit and explicit close; no dangling application pointers in the ISR.
4. Prove the new gateway fits the 128KiB ROM without dropping existing behavior.
   Build using wrapper firmware-check and every profile link guard. Baseline
   reproduced at 130344 bytes; only 728 bytes are currently spare. Keep bulk
   filesystem and CRC logic in the foreground application.
5. Test malformed/bounded/lifecycle/concurrency behavior on host and headless
   target. Freeze exact candidate inputs, preserve recovery, and establish
   firmware deployment authorization before flashing EMOS. Coordinate physical
   qualification through Extender PORT-017. Do not begin graphics/Golem work.

Current phase: provisional implementation, no physical firmware changes.
The latest exploratory wrapper build passes every link guard at 130954 bytes
(118 bytes spare, including its longer UNVERSIONED identity); Core static RAM
ends at BDAAA. Native eZ80 three-byte field loads reduce existing helper cost.
The foreground application builds at 18855 bytes before subsequent refinements.
Host checks cover mailbox guards, keyboard coexistence and the real filesystem
engine with injected short-write, sync, close, read, rename and journal failures.
These are not raw-FAT, emulator or physical qualification. See owning PORT-017
progress notes for the acceptance boundary. Existing QTG/8C remains intact.

Subsequent provisional headless testing passes actual eZ80/EMOS/FatFS raw-image
transfers through 131731 bytes, dropped WRITE response replay, orphan recovery,
STAT/LIST and stage/activation readback. The current application is 18939 bytes;
87 host tests pass. See Extender PORT-017/PROGRESS.md and BOOTSTRAP.md for exact
scope and the initial commissioning request. No physical firmware changed and
no candidate version has been assigned. These maintained changes remain
uncommitted pending Author disposition under the emulator/commit gate.

Commissioning continuation, 2026-09-13 UTC: the Author returned the card and
directed its preparation in response to the concrete candidate-freeze and
version proposal. Freeze v0.1.14 and sdserve-v0.1.0 for that commissioning.
Physical and human acceptance remain pending. The reviewed path bound is 112
bytes for writable targets (120 including the stage suffix); an invalid transfer
returns accepted BAD_REQUEST, reserving STALE exclusively for an unaccepted
session. Host regression covers both. Identified service builds use
`scripts/prepare_sdserve.py`. See Extender BOOTSTRAP.md for guarded installation.

Hardware checkpoint, 2026-09-13 UTC: committed source 71f362a produces
agon-emos-v0.1.14-b2026-09-13-01-07-47Z (130919 bytes) and
sdserve-v0.1.0-b2026-09-13-01-07-48Z (18974 bytes), from clean inputs. Complete
wrapper/link/runtime checks and 89 host checks pass. Author confirms good
physical flash and matching visual observations. The paired console r12 passes
ten physical upload/verify/activation/readback cycles through 131731 bytes.
Extender preserves the source hashes and request audit in PORT-017 evidence.
Actual raw-FAT disk exhaustion also preserves the old target and supports
explicit recovery. Native-keyboard exit/restart is now being observed; do not
close this component task or infer general hardware acceptance before that
result and the Author's confirmation. No further EMOS source change is needed
for these tests; the current physical candidate is unchanged.
