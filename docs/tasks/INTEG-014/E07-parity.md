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
3. [ ] **P03 — Iterate TX candidates.** Build ordinary and bench through all
   canonical link guards; run host and actual-instruction boundary checks. Test
   the first plausible candidate on the frozen diagnostic pair. Measure selected
   endpoint cases early, then exact/mixed and independent captures for retained
   candidates. Reject failures; keep one causal change per candidate and commit
   each disposition. Continue on measured remaining TX costs until parity or a
   documented architectural limit, not merely one successful optimization.
4. [ ] **P04 — Account for and reduce RX work.** Compare stock byte drain/parser/
   effect paths with current generated code. First reduce per-byte call/frame/
   state work; keep register protection and dispatch policy intact. Consider
   cheaper IRQ entry only after the code's actual clobbers are proven. Avoid
   heavy ISR observers that already changed E06 behavior. Retain a C reference
   and meaningful packet/error/ownership tests. No full shared-parser-global reuse
   that corrupts interleaved UART0/UART1 packets.
5. [ ] **P05 — Iterate RX candidates and prove the reverse target.** Measure the
   same return workload against mainboard; use a separately recorded symmetric
   timing extension if coarse ticks cannot decide parity. Validate simultaneous
   streams, callback/register/sysvar behavior, partial/oversize packets, stale
   data, source switching, bounded stalls and ordinary recovery. Remeasure TX
   after RX changes; no one-direction win bought by loss in the other.
6. [ ] **P06 — Resolve residual gaps.** If either target remains unmet, use the
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

1. [ ] **TX02 — Remaining instruction-fetch cost.** After the separate RX01
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

1. [ ] **TX03 — Avoid the temporary status stack slot.** If needed after RX01 /
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
2. [ ] **RX02b — Prove and measure.** Compare full linked IRQ entry/exit against
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
109.608 to **95.528 ms**; Agon-stopped idle falls from69.335 to55.256 ms.
Forward remains591.078 ms versus paired mainboard589.803 ms. No snapshots ran.
The unchanged mainboard return handler reports85.017 ms (different scope from
wire; not a final symmetric parity comparison). Both targets still need work.
RX01 full/mixed controls are running against its immutable installed image.
This measured result activates the already-frozen TX02 and RX02 contingencies.

## TX02 prepared for physical selection

Four in-range JP instructions now use JR, including the hot outer-fault branch;
long branches back to the tick/attempt path remain where required. Both canonical
compositions pass and save8ROM bytes (ordinary130950,122free). All375 complete
sender instruction cases match the original C boundary, including every LSR value.
The instruction count is unchanged: this candidate reduces instruction fetches,
so only the next physical comparison can establish its benefit. RX01 full336
physical cases have passed; mixed controls finish before replacing that image.

## RX01 disposition

RX01 passes376 physical cases (full336/mixed36/wire4), exact data and clean
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
require the exact complete source order and linked guards. All91 host tests now
pass against the freshly prepared ordinary worktree. This is a test-maintenance
commit, separate from TX03's firmware experiment.
