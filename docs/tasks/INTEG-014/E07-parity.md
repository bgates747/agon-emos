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

1. [ ] **P01 — Freeze and preserve.** Snapshot all three involved worktrees and
   generic generated outputs. Freeze existing BENCH integration and accepted
   recovery work separately. Commit this authorization and promote E07P in the
   authoritative INTEG-014 queue before firmware edits. Recheck current hardware
   admission; retain original ROM/ESP/startup hashes and recovery route.
2. [ ] **P02 — Account for the remaining transmitter.** Inspect actual linked
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
