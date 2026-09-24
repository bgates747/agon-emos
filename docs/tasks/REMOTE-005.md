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
