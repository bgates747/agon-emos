# QUAL-002 — Deploy a minimal, recoverable EMOS baseline

## State

- Status: W2 build, automatic checks and graphical emulator pass accepted;
  the Author authorized the reviewed checkpoint commit. Deployment-candidate
  freeze is approved for commit/build and MOS-only flash preparation. W3's stock hardware smoke
  passed by Author report; EMOS installation remains pending.
  WROOM recovery remains a contingency only.
- Started: 2026-09-07
- Finished: --

The Author requested this small deployment milestone after accepting the
simplified harness pinwalk, and specifically required that the earlier EMOS
boot failure not recur. The Author approved the plan and authorized W1's
bounded investigation on 2026-09-07, then authorized W2 preparation. This
does not authorize a physical operation or waive emulator review before commit.

The Author subsequently accepted the proposed identity and directed that the
WROOM remain unwired and its firmware untouched unless recovery is actually
needed. That instruction supersedes this plan's earlier requirement to prepare
and demonstrate the programmer before installing EMOS. Retain the selected
stock payload and documented recovery method; WROOM preparation is not a
normal-deployment prerequisite. Candidate source/link checks remain mandatory.

## Outcome and scope

Install an identified EMOS candidate on the Olimex Agon Light 2 and prove its
ordinary **Legacy-mode** boot: eZ80 EMOS communicates with the stock onboard
VDP over UART0, reads the SD card, runs one ordinary program automatically,
observes the MOS clock advancing, and returns to the MOS prompt.

This establishes the firmware footing for the Author's first complete Extender
mode, **Exclusive Compatible**. EMOS's UART1/P4 transport, mode activation,
parallel transfers, web keyboard input, and P4 display work are subsequent
increments. The onboard VDP remains the display and PB1 VSync source here;
its installed firmware is unchanged.

Reuse maintained EMOS and its ordinary build profile wherever they meet this
scope. A minimal milestone does not require deleting dormant code or reverting
unrelated work. Preserve EMOS ownership of ordinary VDU dispatch and mode
state. Investigate only defects that prevent this baseline from being built,
recovered or tested.

## Starting evidence and the earlier failure

1. [QUAL-001](QUAL-001.md#historical-boot-blocker-correction) records the
   VDP-banner/no-MOS-banner failure. A 24-bit intermediate overflow programmed
   UART0 divisor **11 instead of 1** at the 18.432 MHz eZ80 clock. EMOS waited
   for the onboard VDP's startup reply at the wrong baud and could not execute
   an SD-card recovery command.
2. Commit `0e24b06abdb322fdb4e681a21242ccfebfc8ea65` contains the correction.
   [src/uart.c](../../src/uart.c) widens both operands before multiplication
   in UART0 and UART1. A corrected dirty-source image subsequently booted on
   hardware; that historical result does not qualify the present source.
3. [verify_uart_baud.py](../../projects/emos/verify_uart_baud.py) and
   [its tests](../../tests/test_uart_baud.py) guard the source and final linked
   instructions. Fab passes decoded bytes between processors and did not
   reject the physical baud mismatch. Emulator success cannot replace this
   check or physical boot evidence.
4. The [ordinary profile](../../port/mos-agondev.mk) includes current EMOS
   integration code. [emos_init](../../src/emos.c) selects Legacy, onboard
   VDU and inactive EDU; inspect the complete startup path before relying on
   that state. The fixed parallel qualification profile is unsuitable here.
5. W1 began with `UNVERSIONED-DO-NOT-DEPLOY`. W2 updates
   [port/identity.mk](../../port/identity.mk) to the Author-approved
   `agon-emos-v0.2.0`, initially draft. The former `agon-emos-v0.1.0` candidate
   remains rejected; dirty review builds are not deployment candidates.
6. The Author visually verified stock MOS/VDP before the accepted pinwalk;
   record their exact present versions before this run. The last prepared SD
   autoexec loads `/bin/PWBOOT.BIN` and runs it. That fixture drives GPIO and
   must not execute during the baseline test.

## Work

### W1 — Establish the source baseline and recovery path

1. [x] Inspect the EMOS/build revisions, ordinary startup and VDU route,
   UART initialization and boot scripts. Confirm the divisor correction is
   retained; identify any path that could activate UART1, acquire parallel
   pins or require an Extender peer. Report the smallest necessary change,
   including no code change if sufficient.
2. [x] Identify a known-good stock MOS recovery image, its release and hash,
   and a keyboardless recovery method that works when EMOS never reaches
   the prompt or autoexec. Review the previously successful external ZDI
   recovery and present bench before selecting the method. An SD copy of
   `MOS.bin` alone cannot recover the original failure.
3. [x] Record the selected source/profile and propose its artifact identity
   under the Extender [versioning policy](../../../agon-extender/docs/versions/README.md).
   Obtain the Author's identity approval before stamping a deployment
   candidate. Keep machine-specific programmer and access details in the
   ignored bench record. Report any recovery blocker before accepting the
   candidate preparation for flashing.

**W1 disposition:** Source review and recovery research are complete below.
The Author's existing WROOM board documentation resolves item 2's programmer
selection, and the Author accepted `agon-emos-v0.2.0`. Identity files and
registry updates belong to W2. WROOM attachment, preparation and use are
deferred unless recovery becomes necessary.

### W2 — Prepare and check one candidate and one boot test

1. [x] Make only the changes established by W1. Use the EMOS build wrapper,
   ordinary profile, isolated prepared worktree and project virtual
   environment. Run the repository's applicable tests and configured
   qualification checks, including every registered `FIRMWARE_LINK_CHECKS`
   entry and the final UART0/UART1 divisor guard. Retain logs against the
   selected source and outputs. Do not waive failing gates or edit generated
   source to obtain a candidate.
2. [x] Prepare one bounded test sheet with exact autoexec contents, files,
   expected output, time limits and the documented recovery contingency. Reuse an ordinary
   program or, if necessary, add a small stock-API-only smoke program. It must
   demonstrate SD loading, visible VDU output and MOS-clock progress, then
   return without a keypress. Run `EMOS STATUS` before and after it to expose
   build identity, Legacy, onboard VDU and inactive EDU. No module discovery
   or fake-mode exercise is needed.
3. [x] Prepare the SD-file/autoexec preservation and replacement procedure;
   actual card inspection, backup and staging belong to W3. Account
   for MOS 3's `!boot.obey` and `autoexec.obey` precedence so the test actually
   runs from `/autoexec.txt` under
   [BC-001](../../../agon-extender/docs/qualification/bench-constraints.md).
   Eliminate automatic pinwalk, mode-change or repeated-flash execution from
   the selected boot path. Prepare a stock-compatible invocation of the smoke
   for W3's baseline comparison; reserve EMOS status commands for EMOS.
4. [ ] Complete required Author emulator validation for emulator-coupled
   changes. Prepare the keyboardless update path through working stock MOS;
   do not use the WROOM as the routine programmer. Present the concrete
   candidate procedure and recovery contingency, including
   harness isolation or a verified inactive-pin arrangement, for review.
   Before qualification, obtain the required commit and deployment
   authorization, freeze candidate inputs and build from clean committed
   source. Hash the final binary/ELF and retain proof that the linked UART
   checks inspected those outputs. An earlier artifact's pass does not
   authenticate a rebuilt binary.

### W3 — Deploy and prove the baseline

1. [ ] With the Author's authorization, verify stock boot and the ordinary
   smoke through its stock-compatible invocation. Confirm the exact candidate,
   passing checks, controlled autoexec and reviewed keyboardless update path.
   Retain the identified stock image and recovery reference. Leave the WROOM
   unwired and its firmware unchanged; do not rehearse recovery. Follow current
   local bench instructions for the actual Agon deployment.
2. [ ] Install only the frozen candidate using the reviewed method; verify
   programmed bytes through its supported readback/checksum operation. The
   Author cold-boots the Agon and observes the test. Stop on missing MOS
   startup, timeout, unexpected reset, incorrect mode/route or unexpected
   pin activity. Preserve evidence and diagnose before attempting another
   candidate. If the Agon cannot boot sufficiently to restore stock MOS by
   its normal update path, present the concrete WROOM recovery operation;
   prepare and use it only when needed and authorized.
3. [ ] Record three successful cold boots, exact EMOS/VDP identities, automatic
   SD-program execution, visible output, advancing MOS clock, final
   Legacy/inactive-EDU status and prompt return. Keep raw evidence and hashes
   beside the EMOS test record. Obtain the Author's acceptance of this bounded
   baseline and stop before beginning UART1 integration.

## Completion and evidence limits

The Author has an identified EMOS image running the W3 test on physical hardware,
with the known failure checked in the final image and the documented stock
recovery contingency retained. Recovery need not be exercised for a pass.
Physical boot exercises real UART0 communication with onboard
VDP; it does not measure UART margin or prove complete compatibility.

[QUAL-001](QUAL-001.md) retains broader physical qualification. Its older
[full procedure](../port-203-hardware.md), including interactive keyboard and
fake-mode stages, is not this task's test sheet. This result does not close
unexercised storage, UART-stress or application-corpus gates.
[INTEG-002](INTEG-002.md) retains its parallel work and open evidence; this
milestone neither resumes nor qualifies that transport.

## References and ownership

EMOS owns this task, its firmware and boot-test evidence under
[OWNERSHIP.md](../../OWNERSHIP.md). Extender
[HW-002](../../../agon-extender/docs/tasks/HW-002.md) owns circuit review and
eventual Exclusive Compatible requirements. Its accepted pinwalk verifies
observed lane identities, not firmware boot safety.

Before implementation, review the official read-only `agon-docs` checkout's
`docs/MOS.md` boot-file ordering, `docs/mos/Star-Commands.md` LOAD/RUN behavior,
`docs/mos/API.md` for the smoke program's selected calls, and
`docs/Updating-Firmware.md` update/recovery limits. Record exact API references
when selecting the program; consult release-pinned source where implementation
detail is material. The accepted
[stock communication audit](../../../agon-extender/docs/tasks/AUDIT-004/README.md)
records UART0/onboard VDP and PB1 VSync contracts. Official MOS, VDP and
documentation checkouts remain read-only references.

## W1 findings — 2026-09-07

### Source and build selection

| Input | Inspected selection and boundary |
| --- | --- |
| Maintained EMOS | `58bd9a0d9cf060dc40488f5fb4918cd6b3083190`; source/build files unchanged, with this task's documentation changes pending |
| EMOS profile | Ordinary `port/mos-agondev.mk`; no fixed-qualification macro or fixed adapter source |
| Generic builder | `mos-agondev` `6d4008ce5e01bf3f9a85969cf5fbd06b074fc778`, clean |
| AgonDev checkout | `b67ab2444a63267a42193f204889d466765d8dd2`, clean; installed Clang reports 15.0.7 / LLVM `c76386c0083e6a6236ff774275227e2389f85538` |
| Official MOS reference | Clean v3.0.2 / `8336409351ee5314e02801a7b72a4f1bb5282519` |
| Official VDP reference | Clean v2.16.0 / `c7ac293d2aa81ddfa693390549bcd909069c8fc3` |
| Official documentation | Clean `f9806bd3cbff6ed5d1c08bef1d51fed11764b86b` |

The official release API still selects MOS v3.0.2 and VDP v2.16.0 as latest,
consistent with the accepted audit. No reference checkout was changed.
This selects maintained inputs for W2, not an existing generated image.

1. [Reset initialization](../../src_startup/init_params_f92.asm) puts Port C
   and Port D into GPIO input mode and disables both UART interrupt sources.
   [C startup](../../src_startup/cstartup.asm) initializes BSS/data before
   [main](../../main.c). The initial `emosVduBackend = 0` selects onboard
   UART0 even before the later call to `emos_init()`.
2. `main()` calls both `init_UART0()` and `init_UART1()`. The latter is
   GPIO initialization, not `open_UART1()`: inspected target-header defaults
   set `PC_DR=0xFF`, `PC_DDR=0xFF`, `PC_ALT1=PC_ALT2=0`. Port C remains
   inputs with high output latches. The ordinary boot path does not open
   UART1 or acquire a parallel epoch. Port D starts with the same defaults;
   `open_UART0()` then selects PD0–PD3 for onboard communication, leaving
   the former parallel handshake pins as inputs.
3. `wait_ESP32()` opens UART0 at 1,152,000 baud, 8N1, with hardware flow
   control. [The semantic dispatcher](../../src/serial.asm) sends its General
   Poll and later boot output to onboard UART0. The reply uses the normal
   [UART0 interrupt/parser path](../../src/interrupts.asm). Startup still
   waits for General Poll and mode information; these inherited waits are
   why recovery must work before autoexec. No timeout redesign is proposed.
4. After SD mount/system-variable setup, `emos_init()` selects Legacy,
   onboard VDU, inactive EDU and the unavailable adapter. It does not discover
   providers or contact P4. It does remove the private orphan
   `/.emos-swap.bin`; do not describe EMOS startup as entirely read-only on SD.
   Port C access remains possible through explicit UART1 APIs or explicitly
   entered parallel code, so the controlled smoke must avoid those calls.
5. The main interrupt setup retains PB1 VSync and the existing MOS clock
   handler. This milestone checks clock progress; it does not alter the
   inherited tick accounting or equate that counter to P4 frame timing.
6. The earlier corrective commit is an ancestor of the selected source. Both
   widened baud products remain, and running only the maintained verifier's
   `verify_source()` reports clock 18,432,000, baud 1,152,000, divisor 1.
   The ordinary profile still registers the linked UART guard; the generic
   root build invokes it on the prepared source and final ELF. **No linked
   image was checked in W1.** W2 must build and inspect its actual candidate.

### Smallest W2 changes identified

1. No UART, VDU-route, parallel-engine or startup-control-flow change is
   currently indicated. Retain the ordinary profile and dormant integration
   code; no source rollback or replacement profile is proposed.
2. Replace the identity placeholders after approval and expose the full EMOS
   identity during normal boot, after onboard VDP is available. Currently
   `bootmsg()` identifies MOS lineage; only `EMOS` / `EMOS STATUS` prints the
   EMOS build. Reuse that diagnostic formatting. The existing
   [runtime transcript check](../../projects/emos/verify_runtime.py) hardcodes
   the unversioned identity and must follow the selected candidate identity
   without weakening its check.
3. Prepare the ordinary SD smoke and controlled autoexec described in W2.
   No card was mounted for inspection during W1, so the pinwalk autoexec is
   the last recorded state, not a new media read. For the first boot test,
   recommend operator isolation of the r03 harness from the Agon while
   powered off, retaining the onboard UART0/VDP and PB1 connection. This
   avoids depending on unresolved P4 pin/power state for an EMOS-only proof.
4. The EMOS checkout currently has no `.venv`; establish its project
   interpreter during W2 using the canonical environment convention. W1's
   source-only check used the existing Extender virtual environment and did
   not create a build environment.

### Recovery payload and method

Selected recovery payload: official
[MOS v3.0.2 release](https://github.com/AgonPlatform/agon-mos/releases/tag/v3.0.2),
`MOS.bin`, **108,490 bytes**, SHA-256
`d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae`.
The downloaded bytes match the release API digest and the already retained
generic-builder reference binary. Its release identity is verified; it has
not been flashed or compared with the currently installed Agon flash in W1.

The preferred method is a separate, supported ESP32 running upstream
[agon-recovery](https://github.com/AgonPlatform/agon-recovery/tree/95d68464afd049945e1dfd780c1c07621d565450),
controlled through its USB serial menu. The pinned
[configuration](https://github.com/AgonPlatform/agon-recovery/blob/95d68464afd049945e1dfd780c1c07621d565450/platformio.ini)
targets `esp32dev`; its
[implementation](https://github.com/AgonPlatform/agon-recovery/blob/95d68464afd049945e1dfd780c1c07621d565450/src/main.cpp)
uses GPIO26/TCK, GPIO27/TDI and common ground to the target ZDI connector.
It loads the selected MOS and flash agent from the programmer's SPIFFS,
writes target RAM, runs the flash agent and checks the programmed MOS CRC32.
Its serial menu accepts host input without an Agon keyboard or working MOS.
Select MOS-only recovery; the target onboard VDP remains installed.

That is a contingency method selection using the documented WROOM board, not
a ready deployment procedure. The Author has deferred preparation and wiring
unless recovery is needed. Any subsequently prepared
programmer filesystem must contain the exact selected stock payload above;
do not assume upstream's bundled `data/MOS.bin` has that identity.

The previous
[P4 ZDI diagnostic/recovery](../../../agon-extender/docs/tasks/PORT-008/emos-hardware-diagnostic/README.md)
is explicitly retired. Its source selections and payload generator refuse
execution, and its old image embeds a corrective EMOS payload, not the stock
recovery image selected here. Do not bypass those tombstones or reuse that
image as stock recovery. A maintained P4 replacement would be a separate
bounded implementation decision. Using the target onboard ESP32 for recovery
would temporarily replace VDP, contrary to the present task scope.

Read-only bench inspection found the expected P4 on the Pi and the Agon's
USB serial bridge on the workstation. No serial port was opened and no ZDI
operation was issued. The Author subsequently reported the available WROOM
development board and directed review of the legacy project's existing
documentation and code. Its
[reference index](../../../agon-extender-legacy/REFERENCE.md) leads to the
clean frozen WROOM checkout at
`c200d97fac4027da386c24aecba1f367dd74d329`:

1. The [board record](../../../agon-hardware-wroom-reference/extender/docs/esp32-wroom-32d.md)
   identifies **ESP32-WROOM-32D / 38-pin ESP32-DevKitC**, ESP32-D0WD-V3
   revision 3.0, CP2102 USB bridge, 40 MHz crystal and measured **4 MB flash**.
2. The [pinout](../../../agon-hardware-wroom-reference/extender/docs/esp32-devkitc-pinout.md)
   identifies GPIO26 and GPIO27 on the left header. They match the upstream
   recovery tool's TCK and TDI assignments.
3. Existing [WROOM build configuration](../../../agon-hardware-wroom-reference/extender/wroom_probe/platformio.ini)
   selects `board = esp32dev` and explicitly records the independently
   measured 4 MiB flash. Its
   [UART probe code](../../../agon-hardware-wroom-reference/extender/wroom_probe/src/main.cpp)
   uses the board's UART peripherals and CP2102 console. The
   [combined transport source](../../../agon-hardware-wroom-reference/extender/aew1/wroom_combined/src/main.cpp)
   also assigns GPIO26/27. These are board/code references, not recovery
   firmware to deploy or a reason to import the old transport.

This resolves the hardware-family, flash-capacity and exposed-pin questions
for W1 and selects the existing board for the external recovery plan. The
earlier request to identify those facts again was unnecessary. The board's
current USB attachment, installed image, isolation and ZDI wiring are checks
for a recovery operation if one becomes necessary; historical bring-up is not
a recovery test.
Private device identifiers stay in the ignored local bench record. No legacy
source was modified or copied into the recovery implementation.

### Decision register

| ID | Recommendation and remaining boundary |
| --- | --- |
| QUAL-002-Q001 | Accepted by the Author, 2026-09-07: `agon-emos-v0.2.0` for the ordinary developmental source replacing the rejected predecessor. Start at draft and freeze candidate status only with its reviewed test scope. This does not claim an operational Exclusive mode. W2 stamped the ordinary source and draft registry record; clean candidate freeze remains pending. |
| QUAL-002-Q002 | Resolved for the contingency: use the documented ESP32-WROOM-32D DevKitC with upstream `agon-recovery` if independent recovery is needed. The Author directs no WROOM wiring or firmware preparation/writing beforehand; no recovery demonstration is required before normal EMOS deployment. Keep final candidate checks and a reviewed keyboardless stock-MOS update path as the pre-flash gates. No P4 recovery replacement or onboard VDP change is planned. |
| QUAL-002-Q003 | Accepted by the Author, 2026-09-07: freeze the current mode-3 test sheet as `emos-ordinary-boot-r01`, promote the already approved `agon-emos-v0.2.0` to candidate status, record registry r22, commit the controlled inputs, then build/check and stage the MOS-only one-shot flash. This introduces no transport or fixture-program behavior change. The helper gains clean-input guards; the physical procedure remains the reviewed stock preflight, normal FLASH MOS -f path and three cold boots. |

W1 stops at these findings. No firmware source, build/profile/identity,
emulator, SD contents, device firmware, wiring or power state changed. No
candidate build, physical run, commit or push was performed.


## W2 preparation — 2026-09-07

The [draft boot test sheet](../qualification/minimal-boot/README.md) owns the
exact stock/EMOS scripts, output, time limits, one-shot MOS-only install and
result table. It remains a draft pending review and procedure-identity approval
at freeze. No physical card backup or staging has occurred.

1. Ordinary EMOS source now prints the same complete approved identity at boot
   and in `EMOS STATUS`. No UART, routing, parallel-engine or startup-control
   logic changed. `port/identity.mk` generates one UTC build ID shared across
   nested makes. The fixed parallel composition remains explicitly unversioned.
2. Added a 1,779-byte ordinary MOS application that reads the exact SD fixture,
   observes low-clock-byte progress and returns without input. Its identity
   belongs to the EMOS bundle. Stock and EMOS use the identical program; only
   EMOS media contains the before/after status commands. No fake-mode or
   provider commands are present in this boot fixture.
3. The maintained build wrapper prepares a fresh generated worktree, runs the
   complete configured gates and freezes binary/ELF/map/smoke outputs with a
   manifest. It now gives the generic builder its own project interpreter;
   passing the outer EMOS interpreter previously failed the builder's explicit
   environment check. Both projects retain their own environments.
4. The new identity caused the strict stock shell comparison to fail on that
   exact extra line. The generic builder now accepts a profile-supplied exact
   boot line only once, immediately after the lineage banner. All remaining
   transcript comparisons stay intact. Eleven focused comparator tests include
   missing/wrong/duplicate/misplaced identity and unrelated output. The generic
   change is tracked in its BUILD-001 record and awaits the same human gate.
5. The existing EMOS runtime regression now binds expected identity to the
   frozen firmware hash in the manifest; its discovery, service, fake Dual and
   Legacy transcript checks remain intact on separate regression media. Those
   checks do not authorize or introduce a fake-mode physical boot test.
6. Initial preparation attempts retained their logs: an invalid generated-tree
   destination, the interpreter mismatch, and the expected boot-banner parity
   failure. The first stock smoke attempt exposed an unnecessary `MODE` utility
   dependency on otherwise minimal media; the final scripts omit it. A later
   host assertion needed to account for existing indentation in the VDU status
   line; the firmware output was correct. These were preparation diagnostics,
   not hardware runs. No failing gate was waived.

### Review image and checks

Draft build: `agon-emos-v0.2.0-b2026-09-08-00-22-20Z`.
Local evidence: ignored `build/boot-review-05/`, including the build manifest,
complete qualification/link logs and `review/` smoke transcripts and profile
input records. The review-tool hash is recorded separately so later host-only
assertion corrections do not imply a new firmware image. Both source owners
had uncommitted changes when this image was built and reviewed; this is not
qualification evidence.

| Output | Bytes | SHA-256 |
| --- | ---: | --- |
| Firmware | 116552 | `730cc4995a8c93854c4949143fd13fc8752689e436398feb6f084d3d6f9d277d` |
| ELF | 315260 | `b0d0765b0885da97b9f227746b762b88ec16de0ddb27ac601cf3b289bc492115` |
| Ordinary smoke | 1779 | `160e43b329e6d0f55c9dbd2f457b82506c5188f7f5ce5c48d69cf0d1deb2febb` |

The configured gate passed 133 generic tests, its additional ABI/runtime
checks, stock shell parity, all 66 EMOS tests, provider validation and the
existing target EMOS runtime transcript. Final ABI/VDU and both registered
linked checks passed. UART0 and UART1 use the required 32-bit arithmetic and
**divisor 1**. Headless ordinary stock/EMOS smoke and bad-SD controls pass;
the Author's subsequent graphical review result is recorded below.

The profile launcher also rejects changed firmware, a new overriding boot
file and changed autoexec in disposable negative checks. A separate two-boot
control-flow test under stock MOS and EMOS confirms the rename guard reaches
the following line only once. That test substitutes harmless ECHO for FLASH;
it is script evidence, not a flash rehearsal or programmed-byte verification.

### Install clarification and stopping point

The Author confirmed the normal flash utility supports keyboardless autoexec.
Verified official agon-flash v1.9 source and release bytes: `FLASH MOS <file>
-f` skips confirmation and flashes only MOS, checks programmed-flash CRC, then
resets. The test sheet reuses the previously exercised rename-before-flash
one-shot guard. After reset stops at the consumed filename, the workstation
replaces autoexec with the smoke script while the Agon is off. No custom
flasher, live script rewrite, WROOM preparation or onboard VDP update is needed.

W2.4's emulator, checkpoint and candidate-freeze approval gates are satisfied.
The clean candidate build and final-image checks precede the authorized
MOS-only flash preparation.
No physical SD mutation, serial operation, reset, wiring change or firmware
deployment occurred during preparation or review.

### Author graphical review — 2026-09-07

The Author supplied an emulator screenshot in the conversation. It visibly
identifies onboard VDP 2.16.0 Bistromathics, MOS 3.0.2 Arthur, and the exact
draft build `agon-emos-v0.2.0-b2026-09-08-00-22-20Z`. The screen shows
`BOOT SMOKE SD PASS`, `BOOT SMOKE CLOCK PASS`, the final smoke PASS and the
MOS prompt. Both status reports remain Legacy, registry/generation 0, VDU
route 0, EDU inactive, adapter unavailable and mode generation 0.

Disposition: graphical emulator check **PASS**. The long identity line wraps
at the screen edge but is readable; no output change is needed for this test.
The screenshot is conversation evidence, not a newly captured or hashed local
file. It validates the reviewed emulator behavior; it is not a physical boot
or a measured UART/60 Hz result. No binary or media was changed in recording
the result.

The Author explicitly accepted the emulator pass and authorized committing
the reviewed checkpoint across EMOS, mos-agondev and Extender. The checkpoint
retains the approved v0.2.0 draft source identity and the dirty review image's
original evidence; it does not relabel that image as a clean deployment
candidate. No push or physical operation accompanies the checkpoint. Candidate
status/procedure freeze and a new checked build from committed inputs remain
the next preparation boundary before any physical deployment.

The matching generic boot-identity comparison is committed in mos-agondev
`b0ed60f39e103a1d697094356aa0b4a2ca1909e5`. Use that reviewed builder change
with this EMOS source checkpoint when preparing the next build.

## Physical preflight preparation — 2026-09-07

After pushing the checkpoint, the Author reported both boards powered and the
SD mounted for EMOS preparation. Built and checked the committed inputs with
both dirty flags false, preserved the existing card contents, and staged only
the ordinary stock-MOS smoke. The card was synced and unmounted. Exact hashes,
files and pending observations live beside the test sheet in the
[stock preflight record](../qualification/minimal-boot/stock-preflight-2026-09-07.md).

The new firmware remains draft and is retained on the workstation; no flash
autoexec or new install payload was put on the card. Operator confirmation of
powered-off harness isolation and a physical stock-smoke pass are pending.
Candidate status/procedure freeze still precede EMOS installation. No WROOM
preparation, processor flash, remote reset or power control was performed.

The Author subsequently directed that video mode selection belong exclusively
to autoexec, using `VDU 22 n`; mode 3 is selected for this smoke. Updated the
agent instructions, test sheet and future media generation to prepend
`VDU 22 3`. The fixture program remains unchanged and does not select a mode.
The already unmounted card and retained review media have not been rewritten;
the preflight record preserves their actual LOAD/RUN contents.

After the Author remounted the card, prepended `VDU 22 3` to its stock-smoke
autoexec, preserving a separate backup and operation record. Verified, synced
and unmounted successfully. The preflight record now contains the updated
script and hash; no fixture binary or flash command changed.

## Candidate freeze preparation — 2026-09-07

The Author reports the ordinary mode-3 stock hardware test passed and remounted
the SD. The preflight record preserves that operator report and its limits.
Prepared Q003's narrow freeze: v0.2.0 candidate diagnostics, the revisioned
current test sheet and candidate-build guards requiring clean, unchanged EMOS
and builder commits. The C smoke and MOS/UART/VDU behavior are unchanged.
The registry revision is r22. The Author approved Q003, including the new
procedure identity and candidate freeze, so commit/build/check and one-shot
MOS flash preparation may proceed without another identity approval.
