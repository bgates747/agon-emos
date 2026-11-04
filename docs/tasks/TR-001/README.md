# TR-001: smaller sender candidate and matched test suites

Author authorized implementation after the stock-MOS register-preservation
comparison. Goal: reduce assembled MOS size while retaining background OTIRX
sending and useful application overlap. This is an unpublished trial, not a
replacement for published RC1. Keep main, RC1 tag/assets and old evidence intact.
Work remains on codex/uart0-async; freeze candidate source/builds separately.
Use Eirene's existing AgonDev compiler on Oryx. Heavy Pi activity requires a
Pi5-manager checkout; this task uses only light Pi preparation and Oryx builds.
Any Oryx emulator execution requires its separate canonical FIFO checkout.

The Author additionally requires a candidate AND test suite matching the other
MOS configurations: plain app-name invocation, no switches, the same payloads,
receiver comparator, clocks, calculation kernel, pacing and all three scenarios.

TR-001 [ ] Freeze the plan and RC1/Tom baselines. RC1 source is
47dd56129c0043931bb32f7d329c0392a0b8a809; sender476ROM/11RAM bytes, net480flash.
Tom's original is158instruction bytes plus9inline variable bytes, with12bytes
persistent state including3bytes within the code. Do not use source lines as
size or attribute instruction-level performance to unmeasured behavior.

TR-002 [ ] Implement a smaller MOS path. Preserve syscall inputs and ordinary
interrupt AF/BC/DE/HL behavior as stock MOS does; simplify the implementation,
not that convention. Remove MBASE/IFF and foreign-UART ownership/configuration
validation: foreground ADL1/MBASE0/EI, normal UART setup and sole ownership
become caller preconditions. Retain inexpensive positive-length/BUSY/NULL
semantics where they reuse core state checks. Remove duplicated argument-frame
machinery and saved-IER/active state when ownership of only TX/modem enable bits
suffices. Keep atomic publication, full24-bit counts, last-block clamping,
stock RX chaining, FIFO/CTS checks, bounded CTS deferral/wakeup and drain/restore.
Do not replace CTS deferral with a DI spin without separate Author direction;
normal RX/timer responsiveness remains part of the sender's purpose.

TR-003 [ ] Move the extra IX/IY/alternate-bank callback protection into a thin
application assembly bridge under a clear stock-style raw-callback contract.
Keep ISR primary registers protected through the callback. Include the bridge
in both the C example and new C fixtures; handle a NULL callback without calling
address0. Account for application bytes and call cost, rather than claiming
moved code disappeared. No queues, copied output, new callbacks/tickets or
recovery framework. Explicit caller teardown remains required.

TR-004 [ ] Assemble controlled intermediate variants and the final image on
Oryx. Record per-change text/BSS deltas, full image, metadata/compiler hashes,
source changes and all warning changes. Keep startup/reset/RST18/RX parser
source unchanged. Build a trial-branded candidate reproducibly; preserve RC1
and original scalar/control builds. Do not publish or retag as a new RC yet.

TR-005 [ ] Prepare no-argument focused functional tests and matched PERF2 apps
for both exact published RC1 and trial MOS. Bulk-only, paced-plus-calculation
and bulk-plus-calculation each run3rotated repeats with8192payload bytes,
identical timing core, receiver checks, calibration, checksums and continuing
sequential logs. Verify common compiled measurement objects. The performance
of the required app callback bridge is included in the trial's callable path.
Functional trial checks retain boundaries, zero-length/BUSY/NULL, source reuse,
RX and ordinary-output/finish/re-entry. Context rejection is no longer promised;
never invoke forbidden masked-context calls merely to test a removed check.

TR-006 [ ] Qualify meaningful changed paths before bench use, using finite
actual-firmware Flexe checks on Oryx when available under a lease; preserve any
failure and no model performance claims. The previous emulator waiver concerned
RC1's branding-only final gate, not this changed implementation. Prepare a
Console8 appliance for stock Bistromathics2.16.0 in /test/mos-uart0, with both
matching MOSes, plain commands, concise readme and stock restoration files.
Archive/verify previous card contents before replacement if mounted. Physical
flashing/execution remain Author actions. No protocol or diagnostic VDP change.

TR-007 [ ] Report assembled savings, moved application code, behavior/precondition
changes and test coverage without calling an untested trial approved/current.
Return the concrete fixture and instructions for the physical matched A/B run.
Hardware timing remains the final performance evidence; no improvement is
assumed merely from a smaller module. Leave published RC1 intact.

Expected deliverable: source-pinned trial and matching RC1/trial fixture package,
size ledger, qualification evidence and plain bench protocol. New physical runs
and any release promotion remain subsequent steps, not automatic completion.

Hardware fact checked during implementation: neither the eZ80F92/F93 nor F91
has a documented UART Auto-CTS enable. MCTL bit5 selects multidrop, not automatic
flow control. CTS is sampled through MSR; once loaded, FIFO bytes continue
transmitting even if CTS changes. Retain software CTS checks before each block.
Primary sources: [F92/F93 PS0153, Table63 p119](https://www.zilog.com/docs/ez80acclaim/ps0153.pdf)
and [F91 PS019217-1222, Table105 p190](https://www.zilog.com/docs/ez80acclaim/ps0192.pdf).
