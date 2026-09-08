# Ordinary EMOS boot — hardware smoke PASS

Run `QUAL-002-2026-09-08-01-23-44Z` follows the committed
[emos-ordinary-boot-r01 procedure](../../emos-ordinary-boot-r01.md), frozen at EMOS commit
`e8fd3797e18f77ab2e35d8e66176098af1f668d9`, with the installation deviation
below. **The ordinary hardware smoke passed.** The Author supplied a display
photograph, reported that result on all three cold restarts and confirmed the
original flash succeeded. [Hardware observations and the three-boot table](hardware-result.md)
record what the photograph shows and what depends on the operator report.
The original r01 uppercase MOS argument is invalid; the operator's correction
remains part of this evidence. Artifact status remains candidate.

Candidate: **agon-emos-v0.2.0-b2026-09-08-01-20-50Z**, status **candidate**.
Firmware SHA-256:
`076a9c359591b1beecf4716b9dc41648b8cf3122bf6fd4a1b6dbe56e187c44ba`.

The [build manifest](build-manifest.yaml) records clean source and builder
commits and all output hashes. The complete configured build/check commands
succeeded. [Extracted check output](automated-checks.txt) includes linked
UART0/UART1 divisor 1 and the runtime/parity checks; the four adjacent stock
and EMOS transcripts preserve positive and bad-SD controls verbatim. The CLI's
fake VDP reports unsupported video commands; these checks prove program and
boot-script behavior, not rendering or physical refresh timing. The previously
accepted graphical review and Author-reported mode-3 stock hardware pass retain
their original identities in the task and stock-preflight record.

The workstation preserved the passed stock media, verified the existing
official agon-flash v1.9 and stock recovery payload, and staged only EMNEW.BIN
and [this exact CRLF installation autoexec](install-autoexec.txt). Hash checks,
sync and unmount succeeded. EMDONE.BIN and overriding boot scripts were absent
at handover. Private mount/backup details and complete build logs remain in
ignored `build/boot-candidate-01/` and the recorded local backup directory.

The original installation handover instructed the operator to power off and isolate the Agon
GPIO ribbon, boot the prepared card, observe programmed-flash CRC OK, Done and
automatic reset, then the expected missing-source rename stop. After the Agon
settled, the operator was to power off and remount the card for the candidate
smoke autoexec. That describes the original plan, not verified physical results.

## Operator correction and smoke staging — 2026-09-08 UTC

The Author reported that lowercase `mos` and the payload name without its
leading slash worked. The remounted card contained exactly
[FLASH mos EMDONE.BIN -f](operator-flash-autoexec.txt), with no rename line.
EMNEW.BIN was absent and EMDONE.BIN matched the frozen firmware hash. The
flasher's pinned source confirms its mode arguments are case-sensitive and
uppercase MOS returns an argument error before flashing. The filename is
passed unchanged to fopen; slash removal is observed, not established as a
requirement. See the [task's defect record](../../../../tasks/QUAL-002.md#flash-command-defect-and-smoke-handover).

This is a deviation from the original installation script, preserved alongside
that original script. At this handover, specific flash confirmation was still
pending; the subsequent hardware-result record contains the Author's answer.
The candidate firmware itself has not changed and no reflash is requested.

After backing up the remounted contents, the workstation staged the frozen
candidate smoke and [mode-3 EMOS autoexec](smoke-autoexec.txt). The flash
command is no longer invoked at boot. EMDONE.BIN remains unchanged. Staging
hashes, sync and successful unmount are recorded in the run manifest. The
operator was then asked to perform the procedure's three cold boots with the
Agon GPIO ribbon isolated, observing exact candidate identity, SD/CLOCK/final
PASS, Legacy/inactive EDU and prompt return on each boot. The Author's
subsequent three-boot report is recorded in hardware-result.md.

The [run manifest](manifest.yaml) records the bounded pass and distinguishes
operator reports from the photograph and unmeasured quantities. This is
physical ordinary-boot evidence, not full compatibility qualification. The
completed result is ready for evidence freeze; the candidate bytes remain
unchanged.

## Author acceptance and freeze

The Author accepted correction and freeze after the hardware pass. The
[closeout record](closeout.md) records completion of the bounded milestone and
the corrected r02 procedure. This run remains evidence for r01 with the
recorded operator deviation; no original evidence bytes or build identities
were rewritten to claim an r02 run.
