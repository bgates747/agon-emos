# INTEG-009 Work 1 — Resident keyboard contract

- State: Accepted by the Author on 2026-09-08; Work 2 authorized.
- Prepared: 2026-09-08.
- Owner: [INTEG-009](../INTEG-009.md), Work 1.
- Accepted product decisions: [SETUP-005](../../../../agon-extender/docs/tasks/SETUP-005.md)
  K001–K008 and [REMOTE-001](../../../../agon-extender/docs/tasks/REMOTE-001.md) K004.

## Scope and review distinction

Implement keyboard-source selection in resident EMOS, with its existing MOS
command dispatcher and stock keyboard handlers. The UART wire, CLI names,
autoexec-only persistence and one controlling browser are already accepted.
The concrete transitions, error behavior and first coverage below are the
accepted Work 1 implementation contract. Work 2 implements it without
ExCom display switching or removal of inherited facilities; it does not
choose firmware versions or prepare a bench deployment.

## Commands and observable results

The following table records the original browser increment. The native USB
source amendment below supersedes its `extender`-unavailable row for v0.1.10
and extends reporting/fault cleanup to the selected native source. Historical
v0.1.8/v0.1.9 evidence retains the original behavior.

| Input | Required result |
|---|---|
| `emos keyinput` | Print `Keyboard input: mainboard` or `Keyboard input: browser`; append `(fault)` when the selected browser receiver is faulted. Return success for the query. Do not imply that a browser is connected or has control merely because EMOS selected it. |
| `emos keyinput browser` | Prepare UART1 and the receiver, establish the bounded readiness exchange below, apply retained layout, then publish browser as the sole keyboard source. Print one short confirmation; return success only after local admission commits. |
| `emos keyinput mainboard` | Restore mainboard keyboard admission, release browser-owned held state and relinquish the keyboard service's UART1 ownership. Print one short confirmation. No P4 reply is needed to stop using its input. |
| Re-select the healthy current source | Harmless no-op and the same short confirmation. Re-selecting faulted browser input explicitly attempts recovery. |
| `emos keyinput extender` | Report `Extender keyboard input is not available`; return the existing unavailable result, with no source or UART change. |
| Unknown source, numeric source, switch syntax, or extra argument | Return invalid-parameter with concise usage. No state change. Case-insensitive full names and ordinary whitespace are accepted; no new abbreviation scheme. |
| `set keyboard n` | Keep stock numeric parsing and byte interpretation; apply locale to the selected source and retain the accepted byte in EMOS RAM for source changes. No saved preference file. |

Use existing error values where suitable: `FR_OK`, `FR_INVALID_PARAMETER`,
`EMOS_UNAVAILABLE`, `EMOS_BUSY` and `FR_TIMEOUT`; implementation must verify
those declared names/values before use rather than allocate a new public ABI.
Diagnostics go through current ordinary MOS VDU output, never the keyboard
UART. A failure includes its reason and the source left selected. Autoexec
receives the ordinary nonzero command result; no success-shaped failure.

`emos keyinput browser` is a built-in branch of resident `emos_cmd`, not a
loadable provider. Bare `emos` and existing frozen diagnostics retain their
current behavior. ExCom/Legacy destination commands are recorded product
syntax but complete display-mode implementation is outside this work item.
Existing text/flow/poll probes must report UART busy while the receiver owns it.

## Source and lifetime

1. Cold EMOS startup selects mainboard and does not claim UART1 for keyboard
   service. Ordinary mainboard boot/query traffic and its VBlank clock remain
   as before. Autoexec alone replays desired source/layout selections.
2. Browser selection belongs to EMOS, not to the foreground program or current
   browser tab. The receiver survives command return, ordinary program calls,
   `mos_getkey`/editor waits and display-mode changes. It remains enabled when
   the browser is temporarily unfocused. No per-key poll or diagnostic ACK.
3. Exactly one complete-packet source may update canonical keyboard state.
   Mainboard selected: UART0 keyboard frames are authoritative. Browser
   selected: UART1 key/settings frames are authoritative; UART0 consumes and
   discards only its keyboard frames without invoking callbacks or publishing
   key fields. UART0 display, audio, RTC, mouse and other non-keyboard packets
   retain their existing handling. This does not disable mainboard VDP-local
   physical keyboard effects; no custom VDP firmware is introduced.
4. A source change prepares the target first, then performs held-key cleanup
   and a short protected commit. Reject concurrent transitions. A preparation
   failure retains the previous selected source and releases only resources
   acquired by that failed attempt. Never close somebody else's UART or
   restore a stale vector over a handler this service did not install.
5. Switching to mainboard stops browser admission and releases owned UART1
   interrupt/flow-control resources without depending on P4 responsiveness.
   Its GPIO cleanup follows the proven r03 released/input behavior; it does
   not replay held r02/parallel enable sequences or touch unrelated Port C pins.
6. `emos legacy` does not mean keyboard shutdown. When later mode work uses
   UART1 too, receiver ownership must be shared within EMOS, not closed behind
   an active service. This first keyboard implementation targets normal
   mainboard output; it does not pretend that full mode sharing already exists.

## UART readiness and packet boundaries

1. Use r03 UART1 at 1152000 baud, 8N1, with the accepted CTS and explicit PC2 RTS
   ownership. Prepare receive state and install the handler before admitting
   bytes. RTS denotes receive capacity, not browser authentication.
2. Opening rejects an occupied UART, incompatible vector/pin ownership or held
   parallel ownership. Public UART operations and EMOS diagnostics cannot steal
   bytes, reset the port or close the live receiver. With keyboard service
   inactive, preserve the ordinary UART API behavior. Direct hostile register
   or vector writes remain unsupported, not magically isolated.
3. Use a stock General Poll exchange (`17 00 80 token` hexadecimal,
   reply `80 01 token`) as the bounded start barrier. EMOS discards keyboard
   events before the matched reply. P4 serializes complete frames and places
   already-queued old keyboard packets before that reply, not behind it.
   Drain/reset local pending receive state at admission; no old partial frame
   crosses into the selected source. This is a P4 queue-order obligation, not
   a new wire envelope or a checksum/session-authentication claim.
4. Apply the retained keyboard locale before the readiness poll. A reply proves
   ordered protocol progress, not browser presence, firmware identity or
   semantic acknowledgement of a no-reply locale command. Candidate identity
   remains a separate qualification/deployment check. New browser control
   starts with released state; the UI never replays an old session's backlog.
5. Whole admission deadline: five seconds using the existing MOS
   clock conversion, plus a finite independent poll budget if the clock stops.
   Keep interrupts enabled while waiting. Any setup failure unwinds to the
   prior source. No sleep/retry loop in the receive interrupt.
6. One complete frame at a time reaches dispatch. UART1 has private framing,
   length, payload and error state; it does not overwrite UART0's partially
   assembled payload. Factor keyboard payload handling so both wrappers can
   reuse the stock effect order without sharing assembly scratch state.
7. Key frames must be exactly `81 04 ascii modifiers vkey down`; settings frames
   exactly `88 05 delay_lo delay_hi rate_lo rate_hi led`. Check key-down is 0/1
   and virtual-key index fits the selected stock lookup before calling it.
   Keycode zero remains valid. General Poll is handled privately for admission
   and must not overwrite mainboard General Poll sysvars/flags.
8. Other complete UART1 packet families are consumed without updating mainboard
   canonical display/audio/etc. state in this keyboard-only increment. Unknown
   or oversize frames are discarded by their declared length, never rescanned
   for header-looking bytes inside their body. Invalid keyboard lengths cannot
   invoke a handler using stale bytes.
9. Latch UART line/overrun and receive-overflow faults; stop accepting new keys,
   assert not-ready, reset private partial state and perform local key cleanup.
   Incomplete-frame limit: 250 ms, checked by bounded existing clock
   housekeeping even when the foreground waits for a key. A valid idle stream
   has no timeout. No ISR prints, allocation, filesystem use or blocking TX.
   All UART1 LSR readers must preserve errors for the receiver rather than
   clearing them unnoticed through a transmit helper.
10. Browser source remains selected but reports fault until explicit recovery
    or mainboard selection. Do not silently enable two sources or reset EMOS.
    A silent P4 power loss cannot always be distinguished from a quiet keyboard
    by stock UART alone; this increment adds no heartbeat protocol and makes no
    instant-disconnect guarantee for the physical P4. Browser disconnect is
    detected by P4's network session and uses the accepted release sequence.

## Keyboard effects and cleanup

1. Preserve stock user-vector ABI and interrupt context. The callback receives
   the reusable four-byte payload before ASCII/modifiers/down/count/virtual
   code and keymap/reset effects are updated. Preserve callback-visible edits
   to that payload and documented register obligations. Do not replace this
   with a foreground C callback or poll-only service. Key/settings handling
   sets no new VDP result flag; keycount retains its stock wrap behavior.
2. Track accepted held virtual keys per input owner. On source change or local
   receiver fault, EMOS supplies release events through the same keyboard
   effect path in interrupt context before the new source can publish. Release
   modifiers first so cleanup cannot manufacture Ctrl+Alt+Del. Keep lock state
   distinct from physically held modifier keys. These synthetic events and
   their keycount/callback effects are deliberate recovery behavior, not a
   claim that stock MOS already provides cross-source cleanup.
3. Focus loss, browser disconnect and explicit browser takeover are handled
   by P4: release old-session keys, then admit no more old-session events.
   P4 also releases modifiers before other keys to avoid reset combinations.
   EMOS processes those ordinary key-up packets normally. Source-change
   cleanup must be idempotent when the same releases later arrive from P4.
4. Callbacks must return promptly and may not wait for UART progress while
   interrupts are disabled. Preserve existing callback registration and caller
   lifetime obligations; adding general callback isolation is outside scope.
5. A reboot clears runtime source/session/parser state and returns to mainboard
   startup. Autoexec may explicitly re-enable browser reception. No module
   loader, settings file, relocation or new application gateway is required.

## First supported cases and qualification boundary

The first EMOS receiver proof uses exact stock packets from a controlled P4
sender, then the same receiver consumes the focused browser adapter. It does
not require full ExCom output or a new display fixture in ROM.

| Surface | First cases |
|---|---|
| CLI | Report, select mainboard/browser, repeat selection, mixed case, invalid/trailing tokens, reserved extender rejection; source changes from autoexec. |
| Ordinary input | Letters, digits, space, Enter, Backspace, Escape, arrows, Shift/Ctrl/Alt, ordered down/up, repeats, two held keys and a modifier; `mos_getkey`, editor, sysvars/count and virtual map. |
| Callback | Payload pointer and before-update order, payload edits, registration/unregistration by a scoped test, key-up cleanup and count wrapping. |
| Layout | `SET KEYBOARD` before and after browser selection, then switch back. First mapping examples use stock UK/US; wider layout fidelity is separately qualified, not inferred from forwarding the locale byte. |
| Settings/query | Exact settings packet effects and owned stock locale/query requests. Raw application VDU routing for every keyboard control command is later Work 3; no broad VDU byte scanner is introduced in Work 2. |
| Coexistence | Mainboard display/query/clock progress while browser input is selected; ignored mainboard key packets cannot change canonical keyboard state. |
| Recovery | Busy UART, no/wrong/incomplete poll reply, malformed key frames, line/overflow faults, delayed/partial frames, disable without P4, reset, focus loss and takeover with held modifiers. |

Retain stock Ctrl+Alt+Del handler behavior; intentional reset qualification is
separate from ordinary typing tests. Browser-reserved shortcuts and full
VDP-local paged/control-key behavior remain with REMOTE-001/PORT-005; passing
basic MOS input is not full keyboard compatibility. The first raw control
routing slice must be explicit in Work 3 rather than accidentally sending
P4 keyboard settings to the mainboard display stream.

Autoexec configures video (`VDU 22 3`) before any fixture and performs setup
without keyboard input. Only after an explicit readiness cue may the operator
provide test keys. Exact fixture identities, P4 deployment and capture procedure
belong to later work; none is created by Work 1. A graphical emulator launched
for this review is an attention cue using existing firmware, not new evidence.

## Source findings and narrow implementation touchpoints

Reference baseline: EMOS `97d7dc80ca94e63bc8cb0061721702576d55fc08`, Extender
`d6b8e1d8db33584ccca4a42c7a8bd06a46b280bb`; stock MOS v3.0.2
`8336409351ee5314e02801a7b72a4f1bb5282519`, VDP v2.16.0
`c7ac293d2aa81ddfa693390549bcd909069c8fc3`, docs
`f9806bd3cbff6ed5d1c08bef1d51fed11764b86b`. Official checkouts are read-only.

1. Official [keyboard APIs](https://github.com/AgonPlatform/agon-docs/blob/f9806bd3cbff6ed5d1c08bef1d51fed11764b86b/docs/mos/Keyboard.md),
   [callback API](https://github.com/AgonPlatform/agon-docs/blob/f9806bd3cbff6ed5d1c08bef1d51fed11764b86b/docs/mos/API.md),
   [system variables](https://github.com/AgonPlatform/agon-docs/blob/f9806bd3cbff6ed5d1c08bef1d51fed11764b86b/docs/mos/System-Variables.md),
   [VDP controls/protocol](https://github.com/AgonPlatform/agon-docs/blob/f9806bd3cbff6ed5d1c08bef1d51fed11764b86b/docs/vdp/System-Commands.md)
   were reviewed before implementation details. AUDIT-004's source-qualified
   discrepancies remain applicable; no stock lookup-map rewrite is requested.
2. [mos.c](../../../src/mos.c) `writeKeyboard` forwards a write-only code
   variable through `putch`; it does not retain the layout. Add RAM-only
   retained selection and narrow source-aware routing here. Stock VDP startup
   defaults to UK; do not assume `SHOW Keyboard` reads back a layout.
3. [emos.c](../../../src/emos.c) already supplies the case-insensitive built-in
   namespace. It also contains inherited provider discovery/module machinery.
   The cancellation decision does not mean that historical code vanished.
   KEYINPUT must bypass that loader and require no provider files; broad
   removal/migration of existing provider APIs is not folded into Work 2.
4. [uart.c](../../../src/uart.c) `uart1_claim_rts`, `uart1_receive_ready` and
   `uart1_try_*` require `UART1_IER == 0`. Do not simply enable receive interrupts
   around those polling-only helpers. Supply an owned interrupt-compatible
   path, retaining inactive-service polling behavior and the widened divisor.
5. [interrupts.asm](../../../src/interrupts.asm),
   [vdp_protocol.asm](../../../src/vdp_protocol.asm) and
   [keyboard.asm](../../../src/keyboard.asm) define existing interrupt entry,
   global UART0 parser scratch and keyboard callback/map/reset order. Reuse
   payload effects with isolated UART1 assembly state. EMOS already fixes the
   inherited oversize discard length; stock audit findings must not be copied
   as a description of this fixed EMOS source.
6. [mos_api.asm](../../../src/mos_api.asm) and
   [serial.asm](../../../src/serial.asm) expose raw UART open/read/write/close
   paths. Their ownership checks must cover actual public entry points; merely
   adding a busy check to `emos_cmd` does not protect a resident receiver.

## Review exit

The Author accepted this bounded contract and authorized Work 2. Material
changes to the contract still require review. Work 2 has a separate emulator
review boundary; physical evidence and later work items remain distinct.


## Native USB source increment — PORT-015 W3

Author authorized the ordinary-CLI USB increment on 2026-09-09. Draft
agon-emos-v0.1.10 reuses resident UART1 framing/interrupt effects for the
explicit extender selector. A matched readiness poll commits the requested
source; timeout retains mainboard. Fault cleanup covers either UART1 source.
Mainboard display/clock and the browser-only text gateway are unchanged.

The paired native P4 composition provides only USB acquisition; the browser
composition provides browser acquisition. General Poll remains stock and does
not authenticate firmware/provider identity. Select the documented matching
images. Direct browser/extender cross-selection returns BUSY without touching
the current receiver; select mainboard first until P4 provider switching is
implemented. No new source-selection packet or simultaneous input mixing.

Native SET KEYBOARD additionally waits for an ordered stock poll after locale.
The first P4 composition supports UK/US; other layouts withhold readiness and
cause a bounded fault rather than silently mapping as US. Wider locale,
settings/query/LED and VDP-local control behavior remain unqualified. Host
checks cover extender admission, no-peer rollback, mainboard exclusion,
malformed-frame cleanup, explicit retry and source return. Emulator review,
source freeze and hardware deployment remain separate gates.
