# INTEG-019 — Author-approved flash path amendment

On November 22, 2026, Smith confirmed that the optional P4-to-eZ80 ZDI leads
are disconnected. His earlier wiring confirmation covered only the ordinary
UART/PARLIO harness. He directed proceeding with the standard flash utility
and selected the spare WROOM as the recovery route if an EMOS flash failure
leaves MOS unable to boot. This supersedes the mandatory P4 ZDI preflight and
fallback in the frozen [R02 contract](../INTEG-019.md). Preserve its other
qualification and production handback criteria.

The first preflight independently verified installation of the temporary P4
app, then stopped on ZDI identity `0000/00` where it expected `0007/AA`.
The receiver stopped before the eZ80 dump or any eZ80 write. The diagnostic
worker and children joined. Restore production P4 app0 using the pinned
extent, compare the full readback independently, and prove the production CLI
and services. Preserve the failed preflight evidence without replay.

Install and normally restore EMOS through the commissioned bench runner's
`FLASH mos` path. Preserve production P4 and mainboard VDPlet during the
candidate checks. If an unexpected fault occurs, retain the slot and follow
the bench stop rule. Use the selected WROOM contingency if a failed EMOS
flash leaves MOS unbootable. That contingency needs the actual WROOM setup,
the pinned production EMOS ROM, powered-off ZDI wiring, and full verification
under the [recovery procedure](../../../../agon-extender/docs/mos-recovery.md).
Its prior preparation was unqualified. No WROOM action is part of a normal
successful run. M304 remains held pending candidate qualification.

## Attempt outcome and handback

The standard `FLASH mos` install booted the candidate and the independent
131,072-byte readback matched candidate SHA-256
`a2544d5e158c19f3bdc5b3a69f28afe2472b8c1c5e6235939629953c965b39de`.
The physical keyboard fixture then printed `KEYS FAIL`. Smith reported
pressing A and Enter and seeing the message after Enter. The fixture
printed no make/break pass line and gave no detail on the failed assertion.
The subsequent MOS SD error was its generic display for the fixture's
return status. The watchdog and PARLIO qualification cases did not run.

Smith directed restoring the default bench and checking in. Standard
`FLASH mos` restored production EMOS. The independent 131,072-byte readback
matched SHA-256
`9b90b5d94ec6888d85ef1fa1acf23a82b2e0eb1f179b75f549863fbd72343673`.
Production P4 was independently restored during the earlier recovery from
the ZDI stop. Mainboard VDPlet remained installed. Handback verified a
working Legacy CLI, idle transfers, closed serial handles, and joined
workers. The scheduler confirmed release and TRS-80 received the result.
Retain the failed UART capture and firmware receipts in the local
INTEG-019 evidence. The original keyboard capture SHA-256 is
`5479a49356ef02181a61c4fb13ac73e5547c8801f9d338d2c335449828e8b54a`.

TRS-80 owns the revised unattended test design and acceptance criteria.
Extender owns EMOS and bench execution. M304 remains held pending a
qualification pass.

## Recovery policy review

Smith reports WROOM as the working eZ80 programmer and directs that the
current Extender P4-PC not be treated as qualified for ZDI recovery.
The archived RECOVERY-001 full-ROM success came from an older P4 DevKit
with a different USB identity. The P4-PC attempt here stopped at product
identity with the ZDI leads disconnected. Neither receipt qualifies a
connected P4-PC write. The current
[bench procedure](../../../../agon-extender/docs/operations/olimex-bench/README.md)
retains standard `FLASH mos` and directs separately authorized WROOM
recovery if MOS cannot boot. The shared sparse checkout includes the updated
recovery guide and diagrams. Its host-only operator readiness check passed.
