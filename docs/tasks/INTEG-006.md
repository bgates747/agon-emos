# INTEG-006 — UARTFLOW at 1,152,000 baud

Status: active; reviewed candidate preparation. Started: 2026-09-08.
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
3. [ ] Freeze/build reviewed candidates, preserve rollback, then consume the
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
