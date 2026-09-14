# E07P — Unattended UART parity continuation

## Executive summary

The Author explicitly authorizes an unattended goal run on the hot, fully wired
bench: make Extender transfers as fast as the mainboard before their return or
the available run ends. E07's 49% improvement from the original baseline is
progress, not parity. Continue optimization before advancing to E08 stress or
E09 rendering. Preserve required behavior, demonstrate exact transfers, retain
rollback points, and report an unmet target honestly rather than redefining it.

## Frozen authorization — 2026-09-14

The Author's request:

> “your goal -- make it a goal task -- is to be as fast as mainboard by the time
> i wake up or you run out of tokens. freeze my authorization along with the rest
> of your progress and task contracts ... bench is still hot and all wired up.”

This supersedes the supervised pause before E08 and permits the necessary
EMOS/EDP source changes, builds, controlled physical deployment, SD/CLI use,
Pi reset, capture, and previously authorized firmware recovery within this
throughput goal. Existing normal, ZDI recovery, Pi-reset and sniffer wiring are
available; do not repeatedly ask for permission to use them. No deliberate
bricking or unrelated investigation of the old FLASH failure. No experimental
push. Human gameplay/display acceptance remains distinct from machine checks.
Emulator-specific changes still require their explicit human validation gate.

Use hardware voice only for a genuine physical-assistance blocker or the final
checkpoint; spoken emulator fallback if hardware cannot notify. No routine
milestone wakeups during this unattended run. No subagents are authorized.

## Target and measurement contract

1. Same baud, payload, framing and useful work for both destinations. Preserve
   stock/mainboard behavior; never slow the baseline to manufacture parity.
2. Primary forward target: Extender median elapsed time no greater than the
   paired mainboard median for 65,535 pseudorandom bytes, at least six selected
   observations and three independent wire captures. Report ranges and smaller
   lengths. E07 control is 589.715 ms; Extender is 1,070.062 ms. A 5% improvement
   is the existing candidate-retention floor, not a parity tolerance.
3. Reverse target: same framed return workload received exactly and in order
   at least as quickly as the mainboard. Do not compare P4 enqueue time with
   mainboard blocking-send time. Existing eZ80 completion ticks provide an initial
   comparison; resolve their granularity with a bounded, equally applied timing
   method or repeated workload before claiming close parity. UART1 wire time is
   109.608 ms, including 69.336 ms of Agon backpressure. UART0 wire is not currently
   captured; do not invent a converse wire measurement.
4. Full exact-byte and mixed-transfer controls must pass. Observe keyboard,
   required clock/VDP replies, independent parser state, routing/leases, failure
   cleanup and SD recovery. No clock/keyboard/IRQ disabling as a production fix.
5. EDP remains a port of stock VDP. Retain stock behavior wherever it compiles;
   make the smallest platform adaptations. No renderer redesign, new transport,
   higher baud, unreserved SRAM appropriation, Golem or game feature work.
6. Preserve the five-second whole-call deadline, stopped-clock termination,
   interruptibility, partial-send result, retained byte across stalls, owner/
   error admission and existing packet dispatch semantics. An implementation
   that cannot preserve a boundary needs an explicit recorded contract decision,
   not silently relaxed tests. Compare C first where useful; use targeted
   assembly when measured cost warrants it, not as a blanket language rewrite.

## Ordered execution contract

1. [x] **P01 — Freeze and preserve.** Snapshot all three involved worktrees and
   generic generated outputs. Freeze existing BENCH integration and accepted
   recovery work separately. Commit this authorization and promote E07P in the
   authoritative INTEG-014 queue before firmware edits. Recheck current hardware
   admission; retain original ROM/ESP/startup hashes and recovery route.
2. [x] **P02 — Account for the remaining transmitter.** Inspect actual linked
   E07 C loop/deadline instructions against stock. Extend the pinned CPU harness
   to the full block boundary, including exact port order, timeout, value
   retention, register/IFF preservation and interruptions at relevant boundaries.
   Select a minimal fused/register-based implementation only after identifying
   repeated work; no global interrupt mask around a block.
3. [x] **P03 — Iterate TX candidates.** Build ordinary and bench through all
   canonical link guards; run host and actual-instruction boundary checks. Test
   the first plausible candidate on the frozen diagnostic pair. Measure selected
   endpoint cases early, then exact/mixed and independent captures for retained
   candidates. Reject failures; keep one causal change per candidate and commit
   each disposition. Continue on measured remaining TX costs until parity or a
   documented architectural limit, not merely one successful optimization.
4. [x] **P04 — Account for and reduce RX work.** Compare stock byte drain/parser/
   effect paths with current generated code. First reduce per-byte call/frame/
   state work; keep register protection and dispatch policy intact. Consider
   cheaper IRQ entry only after the code's actual clobbers are proven. Avoid
   heavy ISR observers that already changed E06 behavior. Retain a C reference
   and meaningful packet/error/ownership tests. No full shared-parser-global reuse
   that corrupts interleaved UART0/UART1 packets.
5. [x] **P05 — Iterate RX candidates and prove the reverse target.** Measure the
   same return workload against mainboard; use a separately recorded symmetric
   timing extension if coarse ticks cannot decide parity. Validate simultaneous
   streams, callback/register/sysvar behavior, partial/oversize packets, stale
   data, source switching, bounded stalls and ordinary recovery. Remeasure TX
   after RX changes; no one-direction win bought by loss in the other.
6. [x] **P06 — Resolve residual gaps.** If either target remains unmet, use the
   new instruction/wire evidence to choose the next narrowly scoped candidate.
   Investigate stock-equivalent P4 transport costs only where its backpressure
   is material; wiring only where captures provide reason. Record additional
   numbered candidate steps before implementing discoveries. Recheck this
   contract after each candidate so testing does not replace optimization.
7. [ ] **P07 — Final qualification.** Repeat paired exact/mixed matrices and
   captures; check the ordinary composition and diagnostic overhead explicitly.
   Report timing resolution, variance, bytes/s and flash/RAM budgets. Apply E08
   correctness consolidation as needed; E09 graphics follows only if both pure
   transfer targets are met and sufficient run budget remains. No graphics
   parity claim from pure-data tests.
8. [ ] **P08 — Restore, report and notify.** Preserve final candidates and raw
   evidence; restore the agreed working pre-run firmware/startup, complete ROM/
   ESP verification, actual CLI/SD/keyboard proof and neutral input. Restore
   generic generated outputs after preserving current evidence. Lead the durable
   report with worst-first millisecond/percentage tables and the actual verdict.
   Commit all task-owned work in discrete units; leave unrelated work intact.
   Hardware voice, then stop for Author review. Complete the goal only if both
   transfer targets and required qualification are actually achieved.

Each checked item and candidate disposition gets its own commit. P03/P05/P06
remain open while more optimization is needed. An informative failure belongs
in the results; ordinary setup mistakes need a concise correction, not a new
debug project. Preserve exact images, build identities and local hardware
journals under ignored E07P evidence directories.

## Starting evidence and candidate map

1. [E07](E07.md): 384 exact cases, six large forward samples, three captures;
   82-byte TX leaf, 24 bytes saved, 23.52% less time than E05. Both original ESPs
   and full pre-run EMOS restored. No ZDI recovery was needed in E07.
2. [E03](E03.md) owns the stock-reuse audit; [E05](E05.md) the retained C deadline
   and block changes; [E06](E06.md) the rejected FIFO setting and intrusive-observer
   limitations. Do not repeat those experiments without a new causal question.
3. Current TX still calls clock/deadline and the atomic leaf for each attempt.
   Candidate: keep pointers/count/deadline in registers and combine boundaries
   without changing visible timeout/error behavior. Current RX calls a C byte
   parser for each FIFO byte while the peer is stopped. Candidate: stock-shaped
   drain/parse fast path with C dispatch only for completed packets, if proven.
4. Existing dirty BENCH files and maintained recovery are frozen as separate
   progress commits. This does not make their full host suite green: the known
   source-profile expectation is still a pre-existing mismatch, recorded in E07.

## P01 preservation receipt

Completed before firmware changes: EMOS BENCH integration `7317dca`, Extender
maintained recovery `5a4fd61`, authorization `c72a8b0` / `8afd6ac`.
Ignored `build/integ-014/E07P` preserves patches, selected input archives and
all previous generic generated outputs. Fresh P4 status and keyboard snapshot
in Extender `agents/integ-014/E07P/initial-*` show ready, physically neutral,
no held or pending keys. E07 restoration hashes and the maintained recovery
protocol remain the rollback authority; no bench mutation in this step.

## Candidate TX01 — frozen implementation decision

E07's linked block spills pointer/count/retained byte around both the deadline
call and atomic leaf, in addition to those callees' frames. TX01 replaces only
the private block implementation with a fused register loop. Keep the existing
C block as the explicit host reference; keep the E07 atomic leaf for other
callers. C owns the existing fault-request state via a rare error return.

The private block is reached only from select/layout/text/send, each of which
rejects entry with interrupts disabled and restores enabled interrupts before
sending. A freshly started or previously successful Deadline has elapsed <600;
all failed transmit chains short-circuit. Neither structure is ISR-owned.
These are private preconditions, not new public restrictions. Cache its budget
and last clock in registers, flush at return, and test elapsed only when a tick
changes it. Preserve per-attempt clock read/budget decrement, modulo-8-bit clock
delta, 16-bit elapsed arithmetic, and the original retained byte. Never mask
interrupts across retries or across the whole block. Every port attempt keeps
owner/fault, one acknowledging LSR read, error-before-CTS and THRE admission.

Actual linked-instruction comparisons must cover valid elapsed/budget states,
clock wraps and timeout edges, arbitrary payloads, transient CTS/THRE stalls,
source mutation during a retry, fault/ownership loss, error acknowledgement,
partial output, and ABI/alternate register protection. Public IRQ-off refusal
remains covered by the host sender suite. No hardware deployment before these
checks and both canonical compositions pass.

## P02 / TX01 instruction checkpoint

The linked E07 block takes 6,094,852 interpreted instructions for 65,535 ready
bytes; TX01 takes 2,359,376 (61.29% fewer). All 119 complete-public-sender
comparisons pass, including identical clock reads, deadline bytes, port order,
partial payload and error requests. This is instruction evidence, not timing.
Both canonical compositions pass: bench 130,162 bytes, ordinary 131,006 bytes
(66 ROM bytes free). TX01 grows ROM by 132 bytes; further changes must respect
that narrow budget. Initial assembly branch-range and harness data/stack overlap
were corrected before testing hardware. Existing C uses caller-clobbered IY;
the new private assembly preserves it in addition to the required public ABI.

Host sender and keyboard suites pass both ordinary and telemetry compositions.
TX01 remains experimental pending physical timing and exact/mixed controls;
P03 is still open. No transport or receive code changed in this candidate.

## Receive accounting and next bounded candidate

TX01 hardware preparation uses its immutable clean-commit image `49d31e1`,
identity `agon-emos-v0.1.17-b2026-09-14-12-41-13Z`, SHA256
`ab82bfe7398ea0f616ce66156f5035204bbd206a922e7352d0b4a848ffc0f092`.
The full host suite remains 90/91: the previously recorded profile expectation
omits the already-integrated telemetry unit. This is not a new TX failure.

RX01 will target the private byte parser first, leaving FIFO policy, IRQ register
saves and reply effects unchanged. TX01 links 412 bytes for the C parser with
inlined dispatch and 100 for the UART IRQ drain. Stock uses a short assembly
state machine. E07 return control is approximately 85 ms at the mainboard's
blocking handler versus 109.608 ms measured on the UART1 wire; final close
comparisons still require symmetric timing, as the contract specifies.

1. [x] **RX01a — Parser candidate.** Preserve current byte framing exactly in a
   small assembly state machine, with C called only on complete packets. Keep
   separate UART1 storage (never alias UART0 parser state), exact bad-key-length
   faults, zero/oversize-body behavior, buffer bounds and whole-frame timestamp.
   Make its private C/assembly storage layout explicit and compile-checked;
   retain the original C parser for host and linked-reference comparisons.
2. [x] **RX01b — Instruction and physical disposition.** Compare actual linked
   byte parsing to TX01 across all header/length combinations, boundaries and
   mixed valid/malformed packets. Compare callback arguments, state, stored
   bytes, fault behavior and ABI. Build both compositions and measure only after
   TX01's separate physical result. No reduction in register saves and no peer
   release before parsing are included in RX01.

## Reverse timing resolution decision (P05)

If individual completion ticks cannot resolve the RX comparison, add a separate
fixture mode that performs 16 identical 256-packet returns under one eZ80 clock
interval, using the same sequence on both routes. Keep the endpoint request and
packet format unchanged. Retain all 32,768 returned useful bytes in the fixture's
existing 65,535-byte data array, then validate outside the interval. Copying each
2,048-byte mailbox to that array and preparing each request are included equally
on both routes. No SD access or display output occurs in the interval. Report
this as repeated end-to-end transfers, not inferred wire time or endpoint enqueue
time. One clock tick uncertainty then contributes about 1.042 ms per transfer
at 60 Hz; compare interval bounds, not only rounded medians. If the bounds still
overlap, extend a separately recorded repeat count within available storage or
repeat the test; do not manufacture a parity claim from coarse single samples.

The original app05 exact/mixed/wire cases remain unchanged controls. The timing
extension gets its own immutable fixture build/source and results alongside them.

## RX01a instruction checkpoint

The exhaustive linked parser comparison passes 8,683,703 byte steps across all
256 header values and 256 following length values, concatenated frames,
zero/oversize bodies, fault/ownership guards and key callback mutation. It compares
all private framing fields, all 240 stored bytes and arguments at existing
C effect/service boundaries. Those boundaries are recorded stubs; this does not
replace the host tests of the real cleanup/source behavior or hardware validation.
Interpreted instructions drop from 346,194,452 to 241,951,479 (30.11%).

Both canonical compositions pass; bench is 130,114 bytes and ordinary 130,958
(114 free). Compared with TX01, RX01 saves 48 ROM bytes and uses the same RAM
capacity. All 119 TX instruction comparisons remain identical, including exact
port sequence and deadlines. The C host keyboard/sender suites pass both
compositions. The first harness attempt rebuilt the CPU decoder per byte and
was stopped; reusing each immutable decoder while resetting CPU state runs the
same exhaustive input set efficiently. RX01b physical disposition remains open.

## TX01 first physical observation (not final qualification)

The frozen transmitter-only ROM was independently read back after reboot.
The first app05 wire run passes all four exact cases and independent decoding,
with no snapshot observer and clean Legacy/SD recovery. Forward 65,535 bytes:
P4 **591.051 ms**, paired mainboard **589.871 ms** (+0.20%). P4 wire span is
589.226 ms, including 16.537 ms idle with P4 stopping the sender and 4.356 ms
idle while it permits sending. Reverse is unchanged at 109.608 ms on the wire.
This is 44.76% less forward elapsed time than E07, but not a parity claim.
Raw evidence: Extender ignored `agents/integ-014/E07P/eptw1*`.
The full exact/mixed control follows before changing the installed receiver.

1. [x] **TX02 — Remaining instruction-fetch cost.** After the separate RX01
   observation, if TX parity remains unmet, shorten in-range long conditional
   jumps in the fused TX loop. In particular the per-attempt outer fault branch
   currently uses a 24-bit JP although its destination is nearby. Keep condition,
   deadline and interrupt boundaries unchanged; prove actual linked equivalence
   again. This is a byte-fetch/cycle candidate; instruction count alone will not
   quantify it. Retain long jumps where the assembler requires them.

## Near-parity retention threshold correction

The old 5% candidate-improvement floor is no longer physically attainable for
TX01's 591 ms case: 5% less would be about 561 ms, below the nominal 568.88 ms
needed for 65,535 ten-bit UART characters at 1,152,000 baud. Keep the parity
objective unchanged. For remaining TX candidates require repeatable improvement
beyond observed variance or completion of the strict paired parity target,
with all correctness checks intact. The 5% floor remains useful for coarse
improvements elsewhere; it must not prohibit the final fraction of a percent.
This decision is recorded before implementing the next TX change.

1. [x] **TX03 — Avoid the temporary status stack slot.** If needed after RX01 /
   TX02, classify the single LSR sample into clean-ready, clean-empty, or error
   using its masked value. On clean-ready, sample CTS then write; on clean-empty,
   still sample CTS before retrying; on error perform the same PC_DR/IER stop.
   This preserves the exact port order and clock/owner/fault checks but avoids
   pushing/popping LSR merely to retain THRE across the CTS read. Extend the full
   linked block comparison to all 256 LSR values before physical selection.

## TX01 disposition before RX deployment

All 376 selected physical cases pass: 336 full, 36 mixed and four independently
decoded wire cases; zero data/status/recovery errors. Full large-random medians:
mainboard 589.663 ms (589.524–589.679), P4 591.042 ms (591.039–591.151).
Retain this substantial improvement as the experimental baseline; P03 stays
open because parity is still unmet. Tracked CSVs and metrics are in
[E07P-results/tx01.json](E07P-results/tx01.json). Only one wire capture was needed
to select the next candidate; final qualification still requires three.

The separately frozen RX01 image is next, SHA256
`48e2d07c05b3367f7b9b27f8351cf58378bfeeffeb45a728bf4469d314e8e0a2`,
identity `agon-emos-v0.1.17-b2026-09-14-12-58-02Z` from clean commit `3bdf84e`.
For subsequent host collection, precheck a unique next UDT result path as absent
before launch and fetch that exact new file afterward. This replaces scanning
hundreds of unrelated directory entries; no fixture timing, firmware or wire
procedure changes. Service journals retain every request/receipt.

## RX02 contingency — FIFO loop overhead

1. [x] **RX02a — If RX01 still misses reverse parity, replace only the C drain
   loop with a short assembly loop.** The linked C loop spends roughly 23
   instructions per delivered byte outside the parser, including redundant
   bit/zero tests and saving/reloading its count around the call. A counted
   assembly loop can keep the count in B, push BC as both saved count and the
   existing three-byte byte argument, then use DJNZ. Preserve exact one-LSR-read
   error priority, 16-byte cap, per-byte fault exit, Port C bit masking, and the
   complete existing outer IRQ register save/restore. Keep the C reference for
   host tests. A tiny C completion tail owns RTS reopening and the optional
   telemetry call, so ordinary/bench composition remains profile-controlled.
2. [x] **RX02b — Prove and measure.** Compare full linked IRQ entry/exit against
   RX01 with programmable FIFO/status responses and callback clobbers. Verify
   exact port/read order, byte delivery, cap/error/fault behavior, no premature
   RTS reopening, all primary/alternate registers, SP and IFF restoration.
   Re-run ordinary/bench link guards with a narrow updated Port C owner ledger,
   then deploy only after RX01's isolated physical result has selected this step.
   Remeasure both directions and mixed traffic. No FIFO trigger change, early
   peer release, reduced saved-register set or new wire contract is included.

## RX01 first physical observation

The receiver-only increment passes all four app05 wire cases, independent
bitstream decoding and clean recovery. Reverse UART1 wire time falls from
109.608 to **95.528 ms**; Agon-stopped idle falls from 69.335 to 55.256 ms.
Forward remains591.078 ms versus paired mainboard 589.803 ms. No snapshots ran.
The unchanged mainboard return handler reports85.017 ms (different scope from
wire; not a final symmetric parity comparison). Both targets still need work.
RX01 full/mixed controls are running against its immutable installed image.
This measured result activates the already-frozen TX02 and RX02 contingencies.

## TX02 prepared for physical selection

Four in-range JP instructions now use JR, including the hot outer-fault branch;
long branches back to the tick/attempt path remain where required. Both canonical
compositions pass and save8 ROM bytes (ordinary 130950,122 free). All 375 complete
sender instruction cases match the original C boundary, including every LSR value.
The instruction count is unchanged: this candidate reduces instruction fetches,
so only the next physical comparison can establish its benefit. RX01 full 336
physical cases have passed; mixed controls finish before replacing that image.

## RX01 disposition

RX01 passes 376 physical cases (full 336/mixed36/wire 4), exact data and clean
recovery, with independently decoded wire evidence. Retain the13% receive
improvement; both final parity targets remain open. Details and CSVs:
[E07P-results/rx01.json](E07P-results/rx01.json). RX01b is complete as a candidate
comparison, not a final goal qualification. TX02's immutable image is next,
SHA256 `c56ef6c58dad00b4fd0e19bf83e524a2a9b444a53457607e141754d1b0ba6042`,
identity `agon-emos-v0.1.17-b2026-09-14-13-31-44Z`, from clean commit b0c3e12.
RX02 FIFO-loop source preparation is separate and will not be included in the
TX02 physical image, preserving attribution for each change.


## RX02a instruction checkpoint

The private FIFO drain now uses a counted assembly loop; the original C loop
remains the host reference. Both canonical profiles pass all link guards. Bench
size is 130075 bytes; ordinary is 130920 (152 free). Complete IRQ comparisons
pass 1270 cases in each composition, preserving port order, all saved registers,
stack and interrupt state. A full 16-byte drain uses 284 versus 445 modeled
instructions in bench, 283 versus 443 in ordinary, excluding recorded callback
bodies. All 375 linked TX boundary comparisons remain exact; the nine UART host
checks pass. No physical RX02 conclusion yet: TX02 is installed separately first.


## Host qualification maintenance

The previously recorded 90/91 host result is resolved by adding the existing
telemetry source/object to the explicit ordinary-profile expected lists. The
profile and firmware are unchanged by this correction; the test continues to
require the exact complete source order and linked guards. All 91 host tests now
pass against the freshly prepared ordinary worktree. This is a test-maintenance
commit, separate from TX03's firmware experiment.


## TX03 prepared implementation

The clean-ready LSR path now classifies the single acknowledging sample directly,
avoiding its temporary stack save. Clean-empty still samples CTS; errors stop
the peer and disable IER in the same order. All 375 linked public sender cases,
including every possible LSR byte, pass against the original C implementation.
Ready65535 instructions fall from 2359376 to 2162771; both canonical compositions
pass with no ROM growth (ordinary 130920,152 free). Physical deployment follows
TX02 and RX02 separately; no hardware performance claim yet.


## TX02 initial result and isolation rule

The first exact four-case wire capture gives592.060 ms forward versus the paired
mainboard 589.769 ms; UART1 wire 589.612 ms, permitted idle 4.735 ms. This fails to
improve TX01/RX01 and is being repeated before disposition. The already-frozen
RX02 and TX03 images include TX02 solely to preserve one-change physical
comparisons. If the regression repeats, TX02 is rejected as a performance change;
restore its original long branches in a separately measured TX04 after those
isolated comparisons, before selecting any final candidate. Do not silently
attribute its reversal to a receive improvement or ship it on the strength of
ROM savings alone. No target or correctness boundary changes.


## TX02 disposition — rejected performance hypothesis

Two independent exact captures repeat the regression: P4 592.060/592.063 ms,
paired mainboard 589.769/589.728 ms. UART1 forward wire 589.612/589.581 ms;
RX01 control589.248 ms. Reverse is unchanged95.530 ms. All eight data cases,
independent decodes and recovery pass. Shorter code is not automatically faster
on this physical eZ80/link. Reject this as a speed improvement. The frozen RX02
and TX03 images retain it only for their already-defined isolated comparisons;
remove it before final selection. [Evidence](E07P-results/tx02.json).

1. [x] **TX04 — Restore the four original long branches.** After the already
   frozen RX02 and TX03 comparisons, reverse exactly TX02's four JP-to-JR edits.
   Keep the measured FIFO/parser/status changes unchanged. Recheck canonical
   profiles and the375 complete sender boundaries; compare physical timing and
   exact recovery. This is reversal of a rejected candidate, not a new deadline
   or routing policy. Final selection must include this disposition explicitly.


## TX04 prepared reversal

The four original JP branches are restored in the maintained candidate. Both
canonical profiles and375 complete sender instruction cases pass; ordinary
130928 bytes (144 free), bench 130083. This adds back the eight bytes TX02 saved.
The frozen RX02/TX03 images remain immutable for their separate observations.
TX04 physical disposition and all final parity requirements remain pending.


## P04 accounting checkpoint

The stock/current receive comparison is complete and RX01 has demonstrated a
13% wire-time improvement with 376 exact physical cases. P04 is complete for
accounting and the first proven reduction. P05 still owns reverse parity and
further RX02 measurements; its checkbox remains open. Full register protection,
independent parser storage and existing stock effect adapters are retained.


## RX02 first physical result and conditional RX03

RX02 passes the four original wire cases with independent exact decoding and
clean recovery. Return wire 87.718 ms versus RX01's95.530 ms; Agon-stopped idle
falls 55.257 to 47.445 ms. Forward remains592.092 ms versus mainboard 589.623 ms.
The symmetric batch fixture is now measuring the residual reverse gap; the
mainboard's85.052 ms handler measurement alone is not a scope-matched verdict.

1. [x] **RX03a — Avoid entering an idle optional transmitter.** Only if the
   symmetric return comparison still misses parity, check the existing pending
   length before UART RX completion enters `emos_keyboard_async_irq`. The linked
   function builds a C frame before discovering there is no queued packet.
   Expose its existing volatile length as a private cross-file symbol (same
   storage, no mirrored flag), and guard the call in the existing C completion
   tail. Keep the original callee guard for its other callers. No change to
   queued telemetry, its bounded16-byte drain, error/stall behavior, UART writes,
   RTS release order or ordinary composition. Prefer this small C change over
   another assembly rewrite.
2. [x] **RX03b — Prove and measure that isolated increment.** Extend complete
   IRQ comparisons to run the real idle transmitter and to record queued work
   only when length is nonzero; compare both idle and pending cases, all saved
   registers and port ordering. Existing host telemetry tests must still prove
   real queue/drain/stall behavior. Build both profiles with all guards, then
   measure only after TX04 so receive and transmit effects remain separate.
   Require exact/mixed controls and symmetric reverse timing before retention.


## RX02 symmetric reverse result

Six paired batch intervals verify 393216 useful bytes exactly. Mainboard median
85.417 ms per transfer (85.417–86.458); P4 90.104 ms (89.583–90.625), **5.49% more
elapsed time**. Conservative +/-1.042 ms per-transfer bounds do not overlap in
P4's favor. Reverse parity is unmet, activating the frozen RX03 guard candidate.
The repeated workload includes identical request/arming/copy work on both routes;
it is not the mainboard blocking-handler versus P4 enqueue comparison.
[Batch CSV](E07P-results/ep2rb1.csv), [analysis](E07P-results/ep2rb1-analysis.json).


## RX03a instruction checkpoint

The existing pending length is now a private shared symbol and the C RX
completion tail calls the optional transmitter only when that length is nonzero.
No additional state or public API was introduced. Complete IRQ comparisons pass
3810 bench cases, including real idle execution and pending lengths 1/144, plus
1270 ordinary cases. Idle 16-byte IRQ overhead falls 300→286 modeled instructions;
pending cases add three guard instructions. Both canonical profiles pass:
bench 130089 bytes; ordinary 130928 (144 free), unchanged from TX04 ordinary.
All 91 host tests and375 linked sender cases pass. RX03b physical selection
remains after the frozen TX03/TX04 comparisons; no timing claim from instruction
counts alone. Source and build checkpoints remain separate from deployment.


## Conditional next receive candidate: register argument

The new FIFO loop already holds each received byte in C, but invokes the public
C-ABI parser entry, which reconstructs its stack argument using LD HL,3 / ADD
HL,SP / LD C,(HL). That work is redundant for this assembly-only caller.

1. [x] **RX04a — If RX03 still misses the reverse target, add a private
   register-argument parser entry.** Keep the public C entry and its exact
   fault/owner-before-argument-load order. Add a private entry accepting C with
   the same fault/owner guards, then share the unchanged parser body. The FIFO
   loop still saves/restores BC and calls only this private entry. Do not remove
   guards, change callback/register protection, or expose a new public API.
2. [x] **RX04b — Prove and measure separately.** The public parser must still
   pass the exhaustive8,683,703-byte comparison. Extend the complete IRQ harness
   to observe the private entry's register byte while retaining its existing
   callback/fault/port/IFF checks. Build both profiles, measure against RX03,
   rerun symmetric reverse and forward controls; keep only a demonstrated gain.
   This entry avoids argument marshaling, not any required receive work.


## RX02 disposition

Retain the FIFO-loop improvement after 376 original exact cases (full 336,
mixed36, wire 4) plus the separately scoped12 paired batch rows. Every payload,
status and recovery check passes, with no snapshots during capture. P05 remains
open because the symmetric return comparison is still5.49% slower. RX02b is
complete as a candidate disposition. [Original controls](E07P-results/rx02.json)
and [batch comparison](E07P-results/ep2rb1-analysis.json) remain distinct.
The next installed image is the frozen TX03 status-classification candidate,
identity13:49:22Z, SHA256
`ee66855246b140c96a432cf26c8d85aa11549541579335cc048694769ec5169f`;
it changes only TX relative to the measured RX02 image.


## Existing capture cadence account

Re-reading the same return windows, RX01 has2307 RTS stop intervals and RX02
2308 for 4626 wire bytes: about2.00 bytes per interval. Median asserted time falls
16.917→15.417 microseconds. This is observed RTS cadence, not a separately
instrumented IRQ count. [Counts](E07P-results/return-stop-pulses.json); the local
analysis script and original captures remain in Extender's ignored E07P evidence.
RX03's guard executes after RTS reopening, so faster return may alter batching;
physical measurement must decide its value. Do not infer throughput directly
from its instruction saving or silently change FIFO policy at the same time.


## Smaller forward transfers remain a separate limitation

RX02's full matrix exposes a fixed-cost component as well as bulk throughput:
zero-payload control elapsed0.460 ms mainboard versus 1.886 ms P4;256 bytes
2.805 versus 4.999 ms;4096 bytes 37.317 versus 39.038 ms. These destination elapsed
timers include the established READY/query sequence. The asymmetry in blocking
versus queued READY replies is a known scope component, but these observations
do not identify every cause of the additional latency. [All random lengths and
ranges](E07P-results/rx02-lengths.json). Keep this visible even if the frozen
large-transfer target passes; do not claim parity for every small VDU command.
The objective and baseline are unchanged.


## P06 companion: P4 owner scheduling

A focused read-only comparison found an unconditional one-millisecond sleep in
the current P4 console owner, absent from stock VDP's hardware process loop.
The maintained owner SHA exactly matches the installed diagnostic overlay.
Extender PORT-008 `uart-alignment/E07P-owner.md` now freezes O01–O04 for a separate
stock-alignment comparison after the existing TX03/TX04/RX03 observations. Fix
independent virtual time in the host owner test first; do not alter UART rules,
parser/queue/service logic, task priority, affinity or watchdog policy. Both
stock and current P4 already disable IDLE task watchdogs for the process loop.
This addresses the measured residual setup latency within P06, not a renderer
redesign or a silent change to the paired baseline during EMOS selection.


## TX03 disposition

Retain the status-classification change: all376 original exact cases pass
(full336, mixed36, independently decoded wire4), with clean recovery. Combined
large-random medians are mainboard589.661 ms and
P4 590.063 ms. This is a repeatable improvement from RX02's
592.053 ms but still misses the strict forward target. Return wire remains
87.713 ms, as expected for a TX-only change. [Evidence](E07P-results/tx03.json).
The separately frozen TX04 image now restores the rejected short branches;
SHA256 `6ddc819a8fea7cffe59bdd5d663b7bb53b7e490d1361866ab198802c377c0622`,
identity13:56:56Z, source6057bb6. It will be read back before measurement.


## First forward target observation after TX04

The original long branches restore the expected gain: P4 589.061 ms versus
paired mainboard589.681 ms for65535 exact bytes. Independently decoded UART1
wire time is587.378 ms; reverse remains
87.716 ms. All four original wire cases and
recovery pass, with zero snapshots. This is the first forward target observation,
not final qualification: full/mixed controls are running; six selected samples
and three captures on the final composition remain required. The reverse
matched target is still open. No baseline or threshold has changed.


## TX04 disposition

Retain the original long branches. All376 exact full/mixed/wire cases pass,
with clean recovery. Four large-random observations give P4 median
589.036 ms (589.023–589.061) versus mainboard
589.663 ms (589.558–589.751). This is a
forward win at the candidate stage; the final six-sample/three-capture gate
remains. [Evidence](E07P-results/tx04.json). RX03's already-frozen idle-queue
C guard is next, SHA256
`75c8d6dfb576f815ef91f96a00620871585601ac0cb84bab527ac3da4462e9d2`,
identity14:13:11Z, source d7a3267. The original diagnostic P4 pair stays fixed
through its isolated comparison.


## RX03 disposition — reject as a throughput change

All four original wire cases and twelve matched batch rows pass exact data and
recovery, but return wire87.830 ms fails to improve TX04's87.716 ms. Batch median
89.583 ms versus mainboard85.938 ms still misses parity. Its apparent half-bin
change from RX02 falls within the conservative timing bounds. This does not
establish useful throughput improvement; fewer idle instructions alone are not
retention evidence. Restore only the two RX03 firmware files to TX04. Keep the
stronger independent-idle/pending IRQ harness and the archived candidate for any
future CPU-availability investigation. [Result](E07P-results/rx03.json),
[matched batch](E07P-results/ep3rb1-analysis.json).

RX03's rejection activates RX04. The last retained TX04 image, already verified
and measured on the unchanged ESP pair, is RX04's comparison baseline; supersede
the earlier assumption that RX03 would be retained. RX04 changes only the private
argument path relative to TX04. It will not carry the rejected idle guard.
P4 source preparation may now proceed under O02, but its physical O03 deployment
waits for the new RX04 disposition so the installed firmware factors remain
separate. No reflash of a known rejected receiver merely to repeat its result.


## RX04a instruction checkpoint

Both public and register-argument entries pass8,683,703 linked byte comparisons
against the original C parser, including all state/payload/callback/guard checks.
The private entry avoids three argument-loading instructions for admitted bytes;
its modeled total is216,204,465 versus RX01's241,951,479. The public entry retains
its original fault/owner-before-stack-load order. Complete IRQ comparisons pass
3810 bench and1270 ordinary cases, observing the actual register byte; all375
sender cases and91 host tests pass. Both canonical profiles pass: bench130097
bytes, ordinary130942 (130 free). IRQ counts excluding the stubbed parser are
unchanged, as expected; no physical throughput claim yet. The rejected RX03
idle guard is absent. RX04b measures against the retained TX04 composition.


## Conditional RX05 — caller-established fault admission

The RX04 private register entry repeats a fault test that the masked FIFO drain
already establishes before its first byte and immediately after every parser
return. With interrupts disabled throughout the drain and parser, no other
writer can change that fault between those checks and the private call. The
owner guard is different: a dispatch callback may release ownership without
faulting. Keep that guard. Public C entry keeps both guards unconditionally.
This is a possible remaining per-byte cost, not yet a measured benefit.

1. [x] **RX05a — Prove the private precondition, then prepare if necessary.**
   After RX04's isolated observation, if the reverse target still fails, extend
   the complete-IRQ harness to assert fault-clear at every byte-call boundary
   and exercise callbacks releasing ownership as well as setting faults at
   each byte. Preserve all original public-parser cases. Add a separately
   identified private-entry comparison that follows the actual IRQ caller:
   never invoke its callee after a fault. First run these tests on RX04.
   Remove only the three redundant private fault-test instructions, retain
   owner admission and all public checks, and rerun both canonical profiles,
   full parser/IRQ/sender equivalence and host checks. Commit the proof and
   source separately from any hardware disposition.
2. [x] **RX05b — Isolated physical disposition if still needed.** The already
   prepared P4 owner candidate takes its isolated O03 turn first, with exact
   selected EMOS fixed. If that establishes both targets, do not deploy RX05.
   Otherwise freeze the chosen P4 composition, install and read back the clean
   RX05 ROM, repeat paired reverse batch and wire controls against RX04 on that
   same P4, then full/mixed controls if improved. Retain only a demonstrated
   physical gain with intact services. Do not use an invalid direct call with
   a preexisting fault to excuse a public-API regression; only the private
   masked-IRQ entry has this narrower precondition.


## RX05 precondition proof on unchanged RX04

RX04's exact wire test reduced return wire time to85.960ms from TX04's87.716ms,
but the symmetric batch still misses parity (P4 88.021ms, mainboard86.458ms,
+1.81%). RX05a's preparation condition is satisfied; physical deployment still
waits for the P4 owner comparison. On unchanged RX04, the strengthened IRQ
harness passes3906 bench and1302 ordinary cases, asserting fault-clear before
every parser call, including callbacks setting fault and/or releasing owner at
all sixteen positions. The separate admitted-entry parser test passes8,582,356
byte comparisons. Public parser coverage remains unchanged. This freezes the
caller precondition evidence before removing its redundant private test.


## RX05a prepared candidate

Only the private fault-load/test/return is removed; public fault and owner
checks and private owner admission are unchanged. Both canonical profiles pass:
bench130091 bytes, ordinary130936 (136 free). Public parser8,683,703 cases and
private admitted-entry8,582,356 cases pass against the original C reference;
the admitted path drops from215,900,424 to190,153,356 interpreted instructions.
All3906 bench/1302 ordinary IRQ cases,375 sender cases and91 host tests pass.
This is instruction/behavior evidence only. RX05b remains gated behind the
independent P4 owner comparison and a remaining measured reverse gap.


## RX04 disposition and fixed-ROM owner comparison

All376 original full/mixed/wire cases pass exact bytes and recovery. Return
wire falls87.716→85.960ms, while the matched batch still misses parity at
88.021ms P4 versus86.458ms mainboard (+1.81%). Retain as the receiver candidate
for the next isolated P4 comparison, not as a fully qualified final composition.
Forward four-sample median590.072ms versus589.618ms currently misses its target;
the P4 observations span589.052–590.111ms, including the approximately1ms
scheduling split. Do not conceal that loss behind the reverse improvement.
The minimal owner-loop comparison must remeasure both directions.
[Exact evidence](E07P-results/rx04.json), [batch](E07P-results/ep4rb1-analysis.json).

The actual RX04 ROM is byte-exact, SHA256
`f96f1c22e3dbb7f8900c93bead3a7ff39655c963b139bef7ca3174959fbaac45`,
identity15:17:39Z. Hold it and mainboard01/app05/batch01 fixed for P4 O03.
Prepared RX05 stays off the bench until that separate disposition.


## Conditional RX06 — common payload-state branch

RX04/RX05 still route every admitted byte through both header/length tests
before reaching the common payload state. In framed bulk replies, payload
bytes dominate. A state==2 branch before those tests may save dispatch work
without changing storage, bounds, callback or fault policy. Preserve the old
fallback for all other state values, including unexpected ones; no new state
assumption. Instruction counts are a hypothesis, not a retention decision.

1. **RX06a — Not activated; condition resolved by B02.** Only after RX05's physical result
   leaves a reverse gap or unresolved bounds, put a state==2 fast branch ahead
   of the existing header/length tests. Retain every fallback, public/private
   guard and byte/callback contract. Extend direct parser coverage to seeded
   state values before preparing the candidate; compare against the original
   C parser. Build both canonical profiles and run all instruction/host checks.
2. **RX06b — Not activated; no RX06 source or deployment.** Hold the selected P4 and mainboard
   images/fixtures fixed; install and read back the frozen ROM. Repeat matched
   batch/wire, then full/mixed if improved. Reject a physical non-improvement
   even if interpreted counts fall. Recheck forward parity. If timing alone
   remains unresolved, use the separately frozen symmetric longer-interval
   measurement, preserving raw bytes outside the timed loop.


## P4 owner disposition / RX05 physical gate

P4 owner01 passes376 exact controls,12 matched batch rows, independent wire
decoding and156 seconds of idle with SD online and neutral ready input. Retain
this stock-loop alignment; paired forward588.147 versus589.595ms improves, but
return87.500 versus85.417ms still misses parity. Details and small lengths are
at [the results summary](E07P-results/README.md). RX05b is now released on this
fixed P4 image and unchanged mainboard01. Clean RX05 identity15:43:02Z, SHA256
`60729c6a7bd6d953e2311b933247b56b7e04bbf47c4e374058c8a82df22583ce`.


## First strict matched reverse pass / final qualification sequence

The128-transfer comparison resolves RX05's overlapping short bounds. All
3145728 useful bytes across12 rows are exact, with successful recovery.
Mainboard median86.068ms, P4 85.286ms (−0.91%); conservative mainboard lower
85.938ms exceeds P4 upper85.417ms. [Long evidence](E07P-results/ep5rlong-analysis.json),
[original short control](E07P-results/ep5rb1-analysis.json). Forward's first
RX05 endpoint remains588.132ms. Final qualification is still pending.

RX06 is not activated: the finer symmetric measurement established the target
without another production change. Its conditional outline above is retained
as decision history, not unfinished authorized work. Finish the active RX05
full/mixed controls, then two additional independent wire captures to meet the
six-forward-observation/three-capture gate. Freeze that disposition, install
the clean ordinary RX05 image and repeat exact/mixed, three captures and the
long paired return measurement to measure diagnostic overhead explicitly.
No new candidate unless one of those checks fails or changes the conclusion.
Only then P08 restores exact pre-run firmware and startup, verifies services,
freezes the final report and sends the hardware voice notification.


## RX05 disposition

Retain: all376 original full/mixed/wire controls pass exact bytes and recovery.
Return wire83.952ms improves the fixed-owner RX04 value85.957ms, with unchanged
forward timing. Four forward samples give P4 588.142ms versus mainboard589.690ms.
The128-transfer comparison already proves strict matched reverse parity.
[RX05 evidence](E07P-results/rx05.json). Two final independent captures are
running; ordinary-profile qualification and original restoration remain.


## P05 completion

RX01/RX02/RX04/RX05 account for and reduce receiver work while keeping the
stock effect bridge, independent parser state, full register protection and
required services. RX03 was rejected on physical evidence. The final retained
receiver passes exhaustive public/private and IRQ boundaries, exact/mixed
controls, source/routing recovery and the symmetric long reverse target.
P07 still checks repeatability and the ordinary composition; this checkbox
does not claim graphics parity or final bench restoration.


## P03 completion — six observations / three captures

Final bench RX05 on owner01/mainboard01 passes384 original exact cases and all
three independent captures, with zero snapshots and clean recovery. Six large
forward medians: P4 588.154ms (588.112–588.205), mainboard589.656ms
(589.554–589.766), a0.255% elapsed advantage. The matched long return target
also remains passed. [Final bench composition](E07P-results/rx05.json).
P07's ordinary profile and P08's original restoration are still pending.


## P06 completion

The identified P4 stock-loop divergence and measured RX05 private admission
cost are resolved. Both defined bulk transfer targets pass on the retained
bench composition; no wiring change or unsupported baud increase was needed.
Short-command fixed setup latency remains explicitly reported, and graphics
throughput is outside these pure-data findings. RX06 was not activated. No
further optimization candidate will be started unless P07 changes the result.
