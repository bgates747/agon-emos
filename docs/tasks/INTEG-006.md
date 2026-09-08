# INTEG-006 — UARTFLOW at 1,152,000 baud

Status: complete. Started: 2026-09-08. Finished: 2026-09-08.
Coordinator: [PORT-012](../../../agon-extender/docs/tasks/PORT-012.md).

The Author approved EMOS v0.5.0 for the same bounded Legacy-only UARTFLOW
exchange at the stock MOS/VDP operating baud. Retain v0.4.0 for rollback.
Official MOS API section 0x15 supplies the settings contract; the coordinator
records exact read-only reference commits. Existing widened baud arithmetic
and linked divisor checks cover divisor 1 at 1,152,000 baud. No public API,
transport ownership, frame, deadline or cleanup semantics change.

1. [x] Change UARTFLOW's requested baud and visible announcement; update the
   actual command harness to assert the new value and retain negative cases.
2. [x] Run the complete configured/linked/runtime gate, host tests, same-build
   ordinary/bad-SD smoke and bounded no-peer review. Launch graphical review
   for Author validation before committing.
3. [x] Freeze/build reviewed candidates, preserve rollback, then consume the
   coordinator's physical waveform and endpoint evidence before closeout.

Draft `agon-emos-v0.5.0-b2026-09-08-17-03-35Z` passes full configured,
linked and runtime gates, all 68 host tests, ordinary/bad-SD smoke and bounded
CLI no-peer return. Graphical review is prepared; Author validation is pending.
No SD change or physical deployment occurred. The coordinator preserves the
installed v0.4.0 rollback and owns triggered waveform capture preparation.

## Author review and source freeze — 2026-09-08

The Author supplied the v0.5.0 graphical screenshot showing SD/CLOCK PASS,
1152000 baud, bounded `CTS did not release` and final MOS prompt, then
explicitly approved source freeze and candidate/deployment preparation.
Promote lifecycle metadata to candidate, commit reviewed inputs, and build
clean candidates. This no-peer review does not establish physical baud or
flow-control success. Preserve installed v0.4.0/r01 for rollback.

## Completed — 2026-09-08

The Author confirmed all Agon tests passed and final return to MOS for
PORT-012-2026-09-08-17-17-51Z. Both clean candidates pass the bounded
1,152,000-baud test: exact FLOW/ACK, both deliberate pauses, blocked-sender
cancellation and no late traffic. The full 24 MHz/240M acquisition includes
6.712620 seconds of final quiet; no acquisition exception is needed.

EMOS v0.5.0 and P4 uart-flow-probe-r02 remain candidate identities. This
completes the short target-baud milestone, not sustained-load, analog-margin
or Exclusive Compatible activation qualification. No firmware changed during
closeout. Passing evidence lives beside the r03 hardware design.
