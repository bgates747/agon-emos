# INTEG-009 — Receive stock keyboard packets over UART1

## State

- Status: Keyboard plan accepted for freeze on 2026-09-08; implementation not started.
- Started: -- (implementation has not started).
- Finished: --
- Coordinator: [PORT-008](../../../agon-extender/docs/tasks/PORT-008.md), with
  [REMOTE-001](../../../agon-extender/docs/tasks/REMOTE-001.md) and
  [SETUP-005](../../../agon-extender/docs/tasks/SETUP-005.md) K001–K006.

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
   factoring and synchronization are open K003, not an invitation to rewrite
   all MOS transport or introduce a general service framework.

## Work

1. [ ] Consume K002/K003: explicit test/session entry and exit, selected keyboard
   source, admission/revocation, UART0 coexistence, reset and cleanup. Define
   first supported stock keyboard/configuration cases with the coordinator.
2. [ ] Add resident UART1 reception and reuse stock keyboard handlers with
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

The Author approved the documentation freeze on 2026-09-08 and requested a
stop after committing. Coordinator task order governs only the keyboard slices;
full network, mode lifecycle, parallel provenance and mouse are not prerequisites.
MOS-001 is cancelled; implement resident EMOS extensions instead.
