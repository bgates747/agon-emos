# INTEG-004 — Bounded EMOS UART round-trip diagnostic

Status: implementation authorized, in progress. Started: 2026-09-07.
Coordinator: [PORT-010](../../../agon-extender/docs/tasks/PORT-010.md).

1. [x] Add internal nonblocking UART1 read/write helpers without changing stock
   blocking API semantics. Reject closed or interrupt/flow-controlled use;
   report receive errors distinctly from empty input.
2. [x] Add `EMOS UARTTEST` requiring Legacy and an unused UART1. Send the prior
   18-byte message, receive exact `ACK\r\n`, observe a quiet tail, and always
   close a UART opened by this command. Bound TX/RX/quiet waits and stalled
   clock behavior. Print the result prominently; keep build identity secondary.
3. [x] Test exact reply, silence, truncation, corruption, extra bytes, UART
   errors, busy ownership, clock wrap/stall and cleanup using actual C logic.
   Build through the required profile and linked checks.
4. [ ] Prepare reviewed firmware/emulator and physical qualification artifacts.
   Author approved agon-emos-v0.3.0 for draft review builds; no
   flash before the concrete replacement and recovery path are reviewed.

Official MOS API 0x17 explicitly blocks; serial.asm confirms it never checks
a deadline. MOS C-function slots contain no nonblocking read. Implement this
diagnostic in EMOS Core rather than introduce a client transport bypass or an
unnecessary public API number. The standard UART functions retain their ABI.
The onboard VBlank handler increments the clock by two; use low-byte deltas
and a finite stalled-clock escape, not a claim of millisecond tick accuracy.

## Implemented and checked

EMOS UARTTEST and private UART polling helpers are implemented. Both source
profiles and the ZDS project include the command engine; the fixed parallel
profile receives only the link dependency, without resuming held work.

The configured gate passes (134 generic tests, 67 EMOS tests, target link,
ABI, VDU, baud, shell parity, formatter and FatFS checks). The actual command
engine passes 15 host scenarios. The emulated target times out without a peer
and returns to its prompt; `scripts/review_uart_probe.py` reproduces this and
prepares the graphical review. Its open-stdin handling prevents upstream CLI
EOF shutdown from being mistaken for command completion.

Two build integration findings: use maintained MOS printf, since AgonDev
puts/putchar bypass that path and are excluded from the restricted runtime;
and mos-agondev needed generic RBR/THR/LSR register lvalues (BUILD-002). The
latter now has a target instruction check; official headers stay read-only.

The existing FR_TIMEOUT prints “Volume timeout” after the specific UART
failure, including an autoexec line-number error. It is inherited MOS command
error reporting, not an SD fault. Successful UARTTEST returns FR_OK.

The Author approved agon-emos-v0.3.0 for identified draft review builds.
The initial unversioned outputs are superseded for review. The Author has
supplied the identified graphical-review screenshot; no new-work commit,
flash or SD mutation has occurred.

Identified review build agon-emos-v0.3.0-b2026-09-08-03-42-39Z passes the
complete configured gate and ordinary/bad-SD boot review. The combined
UART profile includes the same-build ordinary smoke, checks its input hashes,
and automatically confirms SD/clock PASS, no-peer timeout and prompt return.
The Author screenshot confirms the exact draft build, SD/CLOCK PASS,
expected no-peer timeout and final MOS prompt. The Author accepted that
visual check and explicitly approved the source checkpoint. Candidate status,
clean rebuild and physical qualification remain pending.

The approved source checkpoint retains v0.3.0 draft identity. It freezes
the reviewed implementation; it does not relabel or deploy the existing
dirty review binary. The source bytes match the graphical-review manifest.

## Hardware candidate preparation — 2026-09-08

The Author authorized preparing the mounted SD for the MOS-only installation,
with both boards powered and the harness seated. Promote the reviewed v0.3.0
source to candidate status, commit that metadata and build from clean EMOS
and builder inputs. No executable source changes accompany the promotion.
The installed receive-only P4 stays in place for the intentional no-reply test.

Use the established two-line rename-before-flash pattern with lowercase mos
and a bare payload filename. Preserve the existing v0.2.0 payload and boot
script before staging. The first handover installs only EMOS; after successful
flash and guarded reset, remount SD to select the accepted combined smoke and
UART timeout script. Do not claim hardware success from media preparation.
The current powered/seated-ribbon instruction overrides the isolation steps
in the earlier ordinary-boot-only procedure; WROOM remains untouched.

Candidate agon-emos-v0.3.0-b2026-09-08-04-04-00Z was built from clean
EMOS daf6881 and builder cf24304 inputs. Full configured and exact-image
ordinary/bad-SD/combined-no-peer checks pass. Extender records SD preparation
in PORT-010-2026-09-08-04-07-39Z beside r03's design tests. The card is safely
unmounted with the guarded installer and preserved v0.2.0 rollback payload;
physical flash, smoke and UART observations remain pending.

The Author reported a successful flash. The remounted SD's consumed payload
matches the candidate. The workstation replaced the guarded installer with
the checked combined smoke/UARTTEST media and safely unmounted SD. The next
hardware observation must confirm SD/CLOCK PASS, bounded no-reply failure and
prompt return. No code or build changed during this second handover.

## Physical smoke and no-reply pass — 2026-09-08

The Author supplied a physical Agon screenshot confirming candidate
agon-emos-v0.3.0-b2026-09-08-04-04-00Z, SD/CLOCK PASS, expected no-reply
failure and final prompt. The Extender
[timeout record](../../../agon-extender/hardware/designs/light2-harness-r03/tests/PORT-010-2026-09-08-04-07-39Z/timeout-result.yaml)
retains the observation and its limits: no measured elapsed time or archived
photo file. The installed candidate requires no rebuild or SD change for the
ACK test. Physical reply receipt remains pending the P4 acknowledgement image.
