# INTEG-009 — Receive stock keyboard packets over UART1

## State

- Status: Work 1 and Work 2 accepted and frozen; Work 3 is next, not started.
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
3. [ ] Prove ordinary MOS key retrieval/editor, key sysvars/count, virtual map
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
freeze on 2026-09-08. Work 3 is the next separate increment and has not started.
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
The Author subsequently explicitly authorized freezing Work 2. Work 3 remains
unstarted; its next proof covers ordinary MOS key retrieval/editor, sysvars,
virtual map, callbacks and applicable keyboard configuration/query routing.
