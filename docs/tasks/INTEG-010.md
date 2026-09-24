# INTEG-010 — Ordinary ExCom console over UART1

## State

- Status: Ordinary ExCom console and native USB input achieved the accepted physical milestone in Extender [PORT-008](https://github.com/bgates747/agon-extender/blob/main/docs/tasks/PORT-008.md#first-ordinary-excom-console-accepted--2026-09-09), including the retained Nurples observation. Wider lifecycle and VDU parity remain open. The preparation notes below describe the original checkpoint; they are not current bench instructions.
- Owner: EMOS resident mode coordinator, UART dispatch and stock reply effects.

## Scope and contracts

Implement case-insensitive EMOS EXCOM / EMOS LEGACY at the idle CLI without
rebooting. Preserve keyboard source/layout and mainboard VBlank. Prepare and
acknowledge EDP readiness before publishing the compatible UART backend; keep
Legacy usable on failed entry. ExCom ordinary RST 10h/18h output reaches the
retained P4 VDU parser. Neither an application transport bypass nor a diagnostic
text echo is the mode implementation. Application display-state preservation,
parallel transport, browser input and full VDP parity remain outside this slice.

PORT-008 in agon-extender owns paired activation, P4 composition and physical
qualification. Its source précis records official MOS v3.0.2 and VDP v2.16.0
contracts. The baseline RST output APIs preserve their register/carry behavior;
UART1 assembles replies separately from UART0 and reuses stock MOS publication.
New resident code is ordinary linked EMOS source, not relocatable modules.

## Work

1. Add bounded transaction control and compatible-UART output to the existing
   coordinator/dispatcher. Never publish an EDP route on General Poll alone.
2. Admit selected EDP display replies without disturbing keyboard callbacks,
   held-key state, source admission or mainboard clock interrupts.
3. Preserve native USB operation in Legacy and ExCom; complete normal return
   and reject absent/unready peers within bounded deadlines.
4. Run source/linked guards, host fault checks and real-EMOS emulator review
   before freezing or preparing guarded paired hardware deployment.

## Evidence

EMOS v0.1.11 draft uses the Author's standing version preapproval; installed
v0.1.10 candidates and rollback images remain immutable. Reviewed software
bundle: `agon-emos-v0.1.11-b2026-09-10-00-50-45Z`.

1. Full qualification wrapper and 76 product tests pass, including real C
   adapter control/staging/private stock scratch/stale-reply/recovery tests.
   Profile-owned linked guards verify the compatible byte/stream register ABI,
   private stock-effect bridge, UART divisor, IRQ and public UART ownership.
2. Paired headless review runs actual EMOS with a native stock VDP renderer
   and maintained P4 lease/key-mapping code. Two round trips preserve input;
   withheld activation leaves a usable Legacy prompt. The Author accepted the final mainboard screenshot and authorized freezing
   this checkpoint. This is not evidence of physical UART timing or the P4 runtime.
3. Extender owns the draft wire contract and hardware sheet. No candidate
   promotion, physical firmware installation or SD write has occurred.

## Implementation notes

UART0 packet assembly stays independent; the ISR saves/restores all 16 stock
payload scratch bytes while applying validated UART1 display effects. Initial
mode/cursor publication is staged until the coordinator commits. Display and
keyboard have separate reasons to retain UART1 ownership. Ordinary display
polls do not reopen keyboard admission or clear held keys.

The compatible assembly bridges preserve the existing RST/C register and carry
contracts. The stock disabled-UART helper was moved before the dispatcher to
keep its original short branches in range. CRC uses a native-width accumulator
so the restricted runtime does not gain an unrelated short-XOR helper. The
held fixed-parallel profile includes these shared resident units but is not
requalified or resumed. Failed active LEAVE retains the selected route;
broader peer-reset/recovery and complete VDU behavior remain future work.

Extender PORT-008 N001 records an intermittent native-reference glyph omission,
reproduced without EMOS and with the native VDP echo confirming complete input.
Control/stream assertions pass; native pixels are not qualified. Mainboard
visual review passed; physical P4 text/cursor checks remain a separate gate.
The current flash image is 130055 bytes, leaving 1017 bytes before the existing
128 KiB bound; later resident additions must continue satisfying that guard.

On 2026-09-09 the Author supplied the final Legacy/Extender-keyboard/MOS-prompt
screenshot, then authorized source freeze and continued preparation. The SD is
with the Author for independent testing. Candidate builds may be prepared
locally; do not change the card or running boards during that testing.

Candidate promotion follows the explicit freeze-and-continue request and standing
version preapproval. Source identity remains v0.1.11; only lifecycle/build
identity changes. Build clean committed inputs through the guarded wrapper.
Physical deployment waits while the Author uses the SD and installed pair.
