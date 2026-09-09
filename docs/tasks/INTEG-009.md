# INTEG-009 — Receive stock keyboard packets over UART1

## State

- Status: Work 1 and Work 2 accepted and frozen; Work 3 accepted and frozen; bounded Work 4 emulator cleanup/recovery accepted and frozen; physical/session cases remain open.
- Started: 2026-09-08 19:18 EDT (Work 1 contract; no implementation).
- Finished: --
- Coordinator: [PORT-008](../../../agon-extender/docs/tasks/PORT-008.md), with
  [REMOTE-001](../../../agon-extender/docs/tasks/REMOTE-001.md) and
  [SETUP-005](../../../agon-extender/docs/tasks/SETUP-005.md) K001–K008 and REMOTE-001 K004.

## Resident implementation boundary

Extend resident EMOS using ordinary compile-time linking and existing command
integration patterns. Keep command parsing, keyboard service state and UART1
interrupt ownership explicit and separable in source. Do not implement an SD
module loader, runtime relocation, transient command provider, or moslet-space
residency restriction. MOS-001's proposed foundation was rejected; this task
has no generic module-system prerequisite. Document actual flash/RAM costs.

## Scope

Add persistent interrupt-driven EMOS-owned reception of stock keyboard packets from P4 over
the existing r03 UART1 at 1152000/8N1 RTS/CTS. P4 obtains focused browser keys;
EMOS preserves the normal MOS keyboard/sysvar/keymap/API/callback behavior.
No application-owned UART interrupt hook, MOS memory writes by P4, raw keymap
transfer, per-key diagnostic ACK or proprietary UART keyboard envelope.
Parallel transport and INTEG-002 are outside this increment.

The existing v0.1.7 gateway is a bounded text diagnostic that closes UART after
transactions. Its passing keyboard-independent evidence is not a persistent
input receiver. Keep ordinary VDU routing and the onboard VBlank clock intact
while the first explicitly entered qualification session proves keyboard
handling. SETUP-005 owns session/source selection and eventual product mode
integration. K005 permits explicitly selected browser input in Legacy while
ordinary mainboard VDU routing continues.

## Accepted CLI contract

Follow the coordinator's ADR-0014 CLI section and SETUP-005 K004: commands and
named arguments are case-insensitive; `EMOS KEYINPUT browser` selects browser
input, `mainboard` selects the Agon mainboard keyboard, and no argument reports
the source. Reserve `extender` as unavailable future hardware input. Keep the
runtime `SET KEYBOARD n` layout separate and consistent across modes.
`EMOS EXCOM` / `EMOS LEGACY` are destination-mode commands, not aliases for
keyboard selection; complete mode transitions are outside this increment.
K005 preserves input selection across display-mode changes, including Legacy.
K006 starts with mainboard keyboard input and restores selections through
`autoexec.txt` only. Add no state/.cfg file or nonvolatile settings store;
interactive commands affect runtime state. On focus loss/disconnect P4 sends
held-key releases, then
stops keyboard packets. No onboard VDP modification is required.

## Bounded reference précis

The accepted AUDIT-004 P013/P014 and A003–A005 traces are the research map.
Official docs reviewed first: `agon-docs/docs/mos/Keyboard.md`, `mos/API.md`
(keyboard APIs, callbacks and sysvars), `vdp/System-Commands.md` (keyboard
controls and return packets). MOS v3.0.2 and VDP v2.16.0 remain clean at the
same audited commits; no upstream source changes are authorized.

1. [Stock MOS packet handlers](https://github.com/AgonPlatform/agon-mos/blob/8336409351ee5314e02801a7b72a4f1bb5282519/src/vdp_protocol.asm):
   `vdp_protocol_KEY` calls the registered key vector before publishing ASCII,
   modifiers, down state, count and virtual code, then calls `keyboard_handler`.
   Preserve callback ABI, interrupt-context constraints, order and lifetime.
2. [Stock keyboard handler](https://github.com/AgonPlatform/agon-mos/blob/8336409351ee5314e02801a7b72a4f1bb5282519/src/keyboard.asm)
   owns the virtual keymap and stock reset-key handling. The map is 16 bytes
   in MOS RAM; `mos_getkbmap` returns its address. It is not sent by the VDP.
3. The stock key event frame is `81 04 keycode modifiers vkey down`.
   [VDP processing/serialization](https://github.com/AgonPlatform/agon-vdp/blob/c7ac293d2aa81ddfa693390549bcd909069c8fc3/video/vdu_stream_processor.h)
   supplies retained event/callback semantics. Browser/P4 message encoding is
   a different interface owned by the coordinating tasks.
4. [VDP keyboard settings](https://github.com/AgonPlatform/agon-vdp/blob/c7ac293d2aa81ddfa693390549bcd909069c8fc3/video/vdu_sys.h)
   return `88 05 delay_lo delay_hi rate_lo rate_hi led`; stock MOS copies
   those five bytes into its settings sysvars and sets no completion flag.
   Key-event handling also sets no VDP result pflag. Keep AUDIT-004's source-
   qualified locale/repeat/query and documentation-discrepancy findings.
5. Stock UART0 and packet-parser scratch state are global. Adding UART1 must
   not splice streams or race common scratch/host state. The receiver's exact
   factoring and synchronization follow the accepted Work 1 contract, not an invitation to rewrite
   all MOS transport or introduce a general service framework.

## Work

1. [x] Consume K002/K003: explicit test/session entry and exit, selected keyboard
   source, admission/revocation, UART0 coexistence, reset and cleanup. Define
   first supported stock keyboard/configuration cases with the coordinator.
   Accepted: [Work 1 contract](INTEG-009/keyboard-contract.md).
   The Author cleared Work 2 to proceed.
2. [x] Add resident UART1 reception and reuse stock keyboard handlers with
   preserved ABI/order. Keep UART1 owned while keys may arrive, including when
   the foreground program waits for input or sends no UART request. Preserve
   independent onboard display, clock and non-keyboard packet handling.
3. [x] Prove ordinary MOS key retrieval/editor, key sysvars/count, virtual map
   and keyboard callback effects. Scope applicable configuration/query routing
   to the selected P4 keyboard source over UART; don't reroute all ordinary
   VDU just to make this test work.
4. [ ] Qualify ordered down/up, held keys/modifiers, repeat, blur/disconnect
   release, stale/partial/overrun data and reboot recovery. Preserve stock
   behavior where intended and document every deliberate recovery deviation.
   Keep normal boot/SD/clock and absent-peer tests before physical deployment.
5. [ ] Use a narrowly scoped autoexec-launched fixture and human emulator
   review, then clean candidates and the coordinator's paired UART evidence.
   New firmware/fixture identities require normal approval; this planning
   update does not select one or authorize a build/flash.

## Review boundary

The Author accepted the Work 2 graphical result and authorized its source
freeze on 2026-09-08, then separately authorized Work 3. Its implementation
and emulator evidence passed Author review. The Author then authorized the
Work 3 source freeze and bounded Work 4 held-key cleanup, truncated-frame
timeout and explicit receiver recovery.
Task order governs only the keyboard slices;
full network, mode lifecycle, parallel provenance and mouse are not prerequisites.
MOS-001 is cancelled; implement resident EMOS extensions instead.

## Work 2 implementation notes

The Author accepted Work 1 and approved `agon-emos-v0.1.8` / registry `r35`
for this resident increment, initially draft. The Author accepted the graphical
result and authorized freezing Work 2. This is a source checkpoint; the reviewed
build retains its draft identity. No physical deployment is part of this step.

1. `emos.c` adds the built-in `keyinput` branch; `emos_keyboard.c` owns the
   selected source, admission, parser, held keys and layout byte. No provider
   files or dynamic loader are involved. Existing provider machinery remains
   outside this bounded implementation.
2. `uart.c` adds a separate resident ownership path. Public UART open/close,
   blocking read/write and polling operations reject access while it is owned;
   the old diagnostics also report busy. Claim rejects occupied UART/vector,
   conflicting pins and parallel ownership. An inactive UART mux left by a
   closed diagnostic is allowed; unrelated Port C bits remain untouched.
3. UART1 has a private five-byte payload and length-counted discard state.
   Admission applies locale and matches stock General Poll privately before
   switching source. UART0's existing parser consumes its own stream and only
   its KEY/KEYSTATE effect wrappers consult the selected keyboard source.
4. The UART1 ISR asserts RTS stop while servicing a bounded FIFO batch. VBlank
   continues its stock clock increment and adds bounded partial-frame/fault/
   source-exit housekeeping. Callbacks remain in interrupt context; the bridges
   preserve primary/alternate registers and protect C's IX/IY frame around the
   stock callback. AgonDev BYTE results return in A, not HL; the linked guard
   explicitly checks that ABI.
5. Held virtual keys, including virtual-code-zero down state, are released on
   exit/fault before a new owner publishes. Physical modifiers are released
   first; synthetic packets clear their modifier bits while retaining locks.
   These releases intentionally affect stock callback/count/sysvar state.
   A callback which deliberately rewrites releases retains responsibility for
   that behavior; cleanup never waits indefinitely for callback cooperation.
6. The stock lookup bounds (248 entries) are checked on wire ingress and before
   map indexing after callback edits. This deliberately prevents malformed
   packets/indices from reading outside the inherited table; the table and
   valid stock handler order remain intact. Wider configuration via raw VDU,
   detailed callback/editor qualification and physical overflow/RTS timing
   remain Work 3/4/5 concerns.
7. The whole-image Port C writer guard now inventories the four resident
   writers and their exact register sequence. Its UART-open rejection check
   recognizes AgonDev's reviewed shared D-register return allocation as well
   as the earlier direct-A shape; neither form may skip ownership rejection.
   The new keyboard linked check binds IRQ saves, BYTE returns, stock effects,
   callback pointer, table bounds and public UART guards to actual image bytes.


### Work 2 draft validation

1. Reviewed build: `agon-emos-v0.1.8-b2026-09-09-00-09-53Z` (draft).
   Firmware SHA-256: `a1e47e52b494aea6845ce1aadb2ab30009fefc64d62eb06e01ef03eaeef896d8`.
2. Repository suite: 75 tests pass, including the maintained receiver under
   ASan/UBSan and actual UART C driver ownership checks. Parser coverage includes
   fragmentation, high-bit payload, 255-byte unknown frames, invalid lengths,
   invalid key values, matched/wrong/missing readiness reply, clock-stall exit,
   source isolation, callback-edited held keys and modifier/zero-code cleanup.
3. Profile-qualified target build passes UART divisor, Port C ownership,
   keyboard bridge, MOS API/VDU and startup checks. The actual image passes the
   keyboard verifier; ten deliberately corrupted ABI/IRQ/ownership bridges were
   rejected. Existing shell parity, VDP framing and C/RST/FatFS regressions pass.
4. Stock and EMOS ordinary SD/clock smoke and bad-SD rejection pass. The new
   eZ80 CLI review passes case-insensitive report/mainboard selection, reserved
   extender and invalid/trailing-argument rejection, two no-peer browser
   timeouts and subsequent mainboard command entry. Mainboard key events reach
   the editor through the refactored stock path. Detailed browser-side keymap,
   callback and settings qualification remains in the later work items.
5. The image is 126359 bytes: 3714 bytes more than the retained v0.1.7 ordinary
   candidate. Initialized data remains 748 bytes; BSS grows by 54 bytes to
   5124. The normal linker/firmware bounds pass. The ZDS project and both
   maintained source profiles include the resident units; only the ordinary
   AgonDev image was exercised here, not ZDS or the held parallel composition.
6. The isolated graphical receiver review was launched for the Author. Expect
   SD/CLOCK PASS, `KEYINPUT FAIL: receiver readiness timeout`, source still
   mainboard, then the ordinary MOS prompt. Fab has no UART1 peer in this
   profile. This is an expected absent-peer test, not proof of P4/browser input.
   The Author accepted this result and authorized the source freeze; the
   reviewed build retains its draft identity and is not a deployment candidate.
   Local bundle/profile/log locations are in the ignored coordinator
   `agents/keyboard/handoff.md`; physical SD and hardware remain unchanged.


The Author subsequently supplied the graphical result for this exact draft:
SD/CLOCK and ordinary smoke PASS, two mainboard source reports, the expected
receiver-readiness timeout, source still mainboard and `/ *` prompt return.
The displayed autoexec error at line 7 and `Volume timeout` are the existing
MOS presentation of `FR_TIMEOUT`; they are expected for the absent-peer case.
The Author subsequently explicitly authorized freezing Work 2. Work 3 was subsequently authorized; its proof covers ordinary MOS key retrieval/editor, sysvars,
virtual map, callbacks and applicable keyboard configuration/query routing.


## Work 3 — ordinary API proof and configuration scope

Author approved `keyboard-api-probe-r01` and registry r36. The implementation
reuses `agon-emos-v0.1.8-b2026-09-09-00-09-53Z` and its original SHA-256
unchanged. No firmware source, version, flash/RAM cost or hardware deployment
changes in this work item. The ordinary [SD exerciser](../../projects/keyboard-api/README.md)
and controlled peer are independent test artifacts.

1. The eZ80 application selects browser input through the resident command,
   installs the stock keyboard callback and observes public sysvars/map APIs.
   The host sends complete stock packets over the emulated UART1; actual
   vector entry, UART driver, resident framing, C bridge and assembly effects
   execute. No test injects guest memory, sysvars, a private function call or
   an application-owned UART handler to manufacture this result.
2. Target assertions cover callback-before-publication and DE payload, callback
   edits to ASCII/virtual code, primary/alternate/IX/IY register preservation,
   modifiers, two independent held keys, 260 packets across counter wrap,
   five-byte settings with no completion flag, getkey ignoring releases,
   editor letters/Backspace/arrows/Enter/Escape, callback removal, retained
   locale across source changes, VBlank progress and return to mainboard CLI.
3. Fab 1.2.3 at `fbb7d7ca887a06966ca8a62ed22272409a5ab640` has UART1 registers
   but no UART1 receive interrupt in `do_interrupts`. Generic mos-agondev
   TEST-001 owns a checked source-copy adaptation and an explicit Unix-socket
   peer for both frontends. It supplies ordered bytes and constant CTS-ready;
   physical baud, FIFO overflow, RTS timing and actual P4/browser behavior are
   outside this proof. Reference sources and the independent Fab fork remain
   untouched. The build retains resolved Cargo lock and executable hashes.
4. The peer observes a fixture-owned one-byte SD stage file. This synchronizes
   tests without adding a keyboard-wire ACK or a product settings store. The
   host enforces a 45-second deadline around the stock blocking input APIs.
   The callback is cleared before returning. A fresh launcher invocation can
   repeat the complete test. The generated profile verifies fixture, firmware,
   peer, runtime manifest, VDP and all immutable media hashes.
5. Fab CLI cannot service display queries while its stdin helper types an
   entire line. The final prompt check therefore waits 500 ms for the stock
   editor's post-prompt mode query before typing its verification command.
   This is a host-fixture pacing constraint, not an EMOS buffering change.

### Applicable configuration/query boundary

The official docs and accepted AUDIT-004 P013/P014 remain the authority.
Work 3 proves the following selected-source subset and identifies the raw-VDU
integration work explicitly; it does not silently claim all keyboard controls.

| Operation | Current ownership and proof |
|---|---|
| `SET KEYBOARD n` / named Keyboard setter | EMOS sends `17 00 81 n` to the selected source and retains the accepted byte in RAM. The peer observes locales 1, 2, then retained 2 after re-entry. Returning to mainboard invokes its separate UART0 sender. Wider locale mapping is not inferred. |
| Readiness General Poll | EMOS's private `17 00 80 token` / `80 01 token` exchange establishes ordered admission; the peer observes two admissions. It is not a settings or session-identity acknowledgement. |
| Incoming `88 05 delay_lo delay_hi rate_lo rate_hi led` | The selected UART1 source updates the normal five MOS sysvar bytes without a key event or new result flag. This fixture sends known values directly as stock status packets; it does not pretend a repeat-control request was routed. |
| Raw `VDU 23,0,&81,n` | Still follows the ordinary output destination. It does not use the source-aware named setter or update EMOS's retained locale. Full source-independent raw routing remains unimplemented. |
| Raw `&88` repeat/LED/query and `&99` key-state query | Still follow ordinary VDU output, presently mainboard. Their mainboard keyboard replies are ignored while browser is selected. Do not use these to claim browser settings/query support in this Legacy test. |
| Raw `&98` control-key enable | Still controls the mainboard VDP's local control-key behavior. Browser/P4 policy and eventual destination routing require coordinator integration; no byte-pattern interception is added here. |

For `&88`, the selected VDP source applies delay 250–1000 ms in 250 ms steps,
rate 33–500 ms, and LED 255 as leave-unchanged; out-of-range delay/rate preserve
existing settings. Thus `(0,0,255)` requests existing settings without changing
them. Stock MOS has no KEYSTATE completion pflag. These are source-qualified
contracts, not operations newly implemented by this fixture.

**W3-K001 — raw keyboard-control routing (open integration boundary):** preserve
these stock requests when input and display owners differ. A correct future
EMOS dispatcher must identify complete commands within the VDU grammar; a
search for `23,0,...` inside arbitrary binary/VDU payload is not sufficient.
Keep this under the coordinator's source/destination integration work. Work 3's
controlled-packet proof does not close this boundary or broaden ordinary VDU
routing. The Author accepted the graphical proof; this open routing boundary
is not closed by that acceptance.

### Work 3 validation

The paired CLI run passes all guest assertions and a subsequent mainboard CLI
command. All 75 existing EMOS tests pass. Exact fixture/runtime/build manifests,
logs and the graphical profile are recorded in the ignored coordinator
`agents/keyboard/handoff.md`. Work 4/5 and physical deployment are not started.


Final Work 3 fixture: `keyboard-api-probe-r01-b2026-09-09-01-03-19Z`, SHA-256
`dfe4e21f491dab9c8f263abfdb7a87fc7a55119de54d0b707c77081bebf3a7ea`.
The graphical peer completes all eleven stages, and the agent observed every
PASS plus the normal mainboard MOS prompt. The Author subsequently accepted the graphical result. The Author authorized its source freeze. The final runtime also passes stock
and EMOS SD/clock smoke and the absent-peer case. A missing socket exits with
status 2 rather than silently falling back.

The graphical run exposed a second Fab limitation: joystick setup holds Port C
inputs high, and the GPIO read ORs UART CTS into them. The explicit test peer
now drives PC3's input level; it does not override a guest-configured output
and leaves absent-peer/ordinary joystick behavior unchanged. This emulator
correction allowed the same EMOS firmware to pass both frontends.


The Author confirmed “emulator review passes” for Work 3. Its bounded
keyboard API proof is accepted; the raw-VDU routing boundary stays open.
Next implementation work is Work 4's receiver cleanup/recovery qualification,
starting with held-key release, truncated-packet timeout and explicit source
recovery. The Author authorized that bounded next increment after accepting Work 3.


## Work 4 — bounded receiver recovery proof (accepted checkpoint)

The Author authorized the next small emulator increment after freezing Work 3.
Reuse the reviewed v0.1.8 firmware and accepted generic UART1 runtime unchanged.
Extend only the ordinary SD exerciser and controlled host peer. The official
Keyboard/API and VDP System-Commands references above were reread first;
stock packet/map/callback behavior remains the authority. Synthetic cleanup and
partial-frame expiry are EMOS's accepted recovery contract, not stock promises.

1. Exercise repeated downs while Shift and two keys are held, then select
   mainboard. Observe modifier-first synthetic releases through the registered
   callback, empty map, count increments, cleared physical modifiers and
   retained Caps Lock. Repeated selection/later ticks must not repeat cleanup.
2. Before a new matched poll reply, the peer sends stale keys, a wrong token
   and further keys. None may publish. Leave the admitted receiver idle beyond
   250 ms, then prove that a fresh key still works; idle is not partial data.
3. Hold Ctrl, a normal key and a virtual-code-zero key, then trickle a key
   frame at 100 ms intervals. Whole-frame expiry must release the held keys
   before completion. The late tail and later complete keys must remain
   ineffective while faulted. Explicit browser selection must perform a fresh
   readiness exchange and accept new keys, followed by mainboard prompt return.

This does not finish all of Work 4. Actual P4/browser blur, disconnect, takeover,
restart and physical UART overrun/RTS behavior still require their owning peer
and hardware tests. The socket runtime cannot manufacture meaningful physical
line/overflow evidence. Source-aware raw VDU routing remains W3-K001. No EMOS
firmware rebuild, SD-card write, P4 deployment or physical test is in this slice.
The Author approved keyboard-api-probe-r02 and registry r37. The bounded
emulator proof passes automated checks and supplied screenshot review; the
Author subsequently authorized its source freeze. Do not mark the whole work item done.


### Work 4 bounded validation

Author-approved fixture `keyboard-api-probe-r02-b2026-09-09-01-54-03Z`, SHA-256
`d8ba03877fb1bbc261260c487e080bf4883e10fe21e37e818698bda6a9af6caa`,
passes all nineteen paired CLI stages and a subsequent mainboard CLI command.
The same accepted EMOS v0.1.8 image and runtime hashes are retained. No firmware
source or linked executable changed. The build uses warnings as errors.

The peer records five distinct readiness tokens and locales `[1,2,2,2,2,2]`.
Target checks prove modifier-first cleanup once, preserved lock state and
callback/count effects, stale keys around a wrong readiness token discarded,
idle without a fault, and partial timeout with Ctrl/normal/zero-code cleanup.
The target reached its fault result while the packet's tail was still pending;
complete subsequent keys had no effects before explicit retry. These are
functional clock/receiver observations, not physical UART timing qualification.

Python execution, controlled-input hashes, diff checks and the coordinator's
registry/templates/VDP identity checks pass. Existing stock/EMOS boot and
absent-peer runtime evidence remains applicable because both images and the
runtime are byte-identical. This increment's autoexec also passes SD/CLOCK.
The generated review profile retains an immutable peer-script copy so later
fixture edits do not alter this review. Exact local paths and manifests are
recorded in the coordinator's ignored keyboard handoff.

The supplied screenshot confirms the graphical result. The Author subsequently
authorized its source freeze. Work 4's wider physical/session cases remain open.


The graphical profile was launched for review. Its controlled peer also reports
all nineteen stages PASS, and the session subsequently exited cleanly. The
Author then supplied the matching r02 build screenshot: all four recovery PASS
lines, the intentional browser fault followed by successful retry, final API
PASS, mainboard input and the normal `/ *` prompt. This confirms the bounded
emulator review result. The Author subsequently authorized source freeze; no
physical work or next implementation increment is authorized by this freeze.


The bounded Work 4 checkpoint is frozen after Author screenshot review and
explicit commit approval. The nineteen-stage fixture and peer sources still
match the reviewed manifests. The reviewed EMOS/fixture builds retain their
draft identities; clean physical candidates and the real P4 sender remain
future work. Keep Work 4 unchecked for its remaining session/hardware scope.
