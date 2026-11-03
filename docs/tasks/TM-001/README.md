# TM-001: minimal interrupt-driven UART0 sender for Eirene MOS

We want applications to spend less eZ80 time sending bytes to the VDP, leaving
more time for their actual work. Start with Tom Morton's small candidate, fix
its correctness issues with the lightest practical changes, and add bounded
16-byte OTIRX refills. Do not port the larger experimental implementation as
the starting point. Its refinements and evidence remain available for later use.

**Status: plan updated for Author review; implementation paused.**
The Author chose this direction on 2026-11-03 and requested a pause after this
update. No implementation, emulator execution, SD deployment or flashing is
part of this planning update.

## Repository and interface decisions

Canonical working repository: `/Agon/mystuff/eirene-mos`, branch
`codex/uart0-async`, tracking `bgates747/eirene-mos` with upstream
`AgonPlatform/eirene-mos`. Starting commit:
`86b694260e3be3b391284ce4aa8ecc758f7f34e9`.
Eirene already uses AgonDev; retain that toolchain for these firmware changes.
The earlier ZDS II MOS remains historical/reference work, not this branch's
implementation base. Future TM work and its records belong here.

Keep ordinary RST18 synchronous and unchanged. Add an explicit new MOS syscall
for asynchronous sending; inspect Eirene's existing interfaces and allocate an
unused entry before specifying its number. Do not import our old UTX1 namespace
or copied-RST18 mode by assumption. Tom's attachment itself adds no syscall: its
send routine is an ordinary application assembly call, and its init/exit use
MOS's existing vector-setting syscall. His later recommendation is to expose
that facility as a new MOS syscall instead of overloading RST18.

The initial contract is one borrowed buffer outstanding at a time, not our
three-descriptor queue. The app may calculate or prepare a different buffer
while MOS transmits; it must retain the submitted source unchanged until
completion. A second submission while busy must not overwrite the active
transfer. Keep the completion interface small; no per-write ID system is needed
merely to distinguish a single outstanding transfer. A callback means MOS has
finished reading the source, not wire drain or VDP command completion.

Normal startup and stock output remain proven paths. Activate the experimental
sender only through explicit application/CLI action; preserve normal MOS receive
processing while it is active. Do not add recovery frameworks or unrelated
robustness machinery to this minimal experiment.

## Existing identifiers and evidence

TM-001 [x] Preserve Tom's original and establish its one-transfer contract.
The exact attachment is [reference/async_vdu.asm](reference/async_vdu.asm), SHA256
`93e5ddad8e72f1b22541c16e15cd0600e820204f02e13a559c2a6f6b423f5bd4`.
No third-party license grant is inferred from the attachment.

TM-002 [x] Audit the previous implementation against Tom's candidate. Preserve
[the earlier comparison](reference/previous-comparison.md) as historical findings;
its recommendation does not override this updated scope.

TM-003 [x] Build the earlier standalone comparison prototype. This means compile
only: its app and six stopped Flexe profiles are preserved under the old
`agon-mos/build/uart0-tomm/run001`. They have not executed. Adapt their useful
workload/comparator code to the new Eirene design rather than treating them as
qualified new-branch binaries.

TM-004 [ ] Run the revised finite comparison in Flexe on both engines after the
minimal implementation and fixture are ready. Follow the shared Oryx FIFO,
receiver verification and normal-return checks. Preserve any failing original
case and identify each fix. Old reservation `c469951ff727` was cancelled during
the repository transition; no reservation is currently held by this task.

TM-005 [ ] Prepare and run matched Console8 paced/useful-work and bulk benchmarks.
Physical timing decides CPU/headroom and throughput claims. Include receiver
byte verification, repeatable inputs and sequential reports. Physical firmware
flashing remains an Author action; reserve no hardware test or SD deployment
through this task update.

TM-006 [x] Record the selected delivery direction: Tom-first, light fixes,
minimal OTIRX, explicit new syscall, unchanged RST18. Wider refinements remain
available but are not requirements for the first implementation.

## Minimal implementation tranche — pending review

MI-001 [ ] Inspect Eirene's current UART ISR, RX injection/parser facilities,
vector installation and syscall allocation. Reuse what it already supplies;
do not reproduce the old MOS adapter or assume its keyboard-hook restrictions
necessarily apply to Eirene. Write the small single-transfer calling contract.

MI-002 [ ] Adapt Tom's sender with narrowly justified fixes: arm the example's
completion flag before submission; reject zero length and overlapping sends;
close the interrupt-vector installation race; define callback register/context
requirements and a small save/restore boundary; finish accepted traffic before
teardown. Replace unbounded CTS/FIFO spinning inside DI with the smallest
correct deferral/wakeup mechanism that keeps normal receive service alive.
These are normal-use correctness fixes, not a request for an expanded framework.
Record the source delta and justify each addition.

MI-003 [ ] Replace the scalar FIFO-copy loop with OTIRX, clamping the positive
count to remaining bytes and the 16-byte FIFO capacity. Account source/count
once per block. Preserve the before-block CTS check; do not silently reintroduce
per-byte CTS polling. Treat receiver reserve as a qualification question, not a
promise made by the instruction. Keep the lightly fixed scalar version as a
matched control to isolate OTIRX from other changes.

MI-004 [ ] Integrate through the new opt-in Eirene syscall, preserving startup
and ordinary RST18 behavior. Keep source-release, local wire drain and remote
execution distinct. Report source/ROM/RAM size and hot-path changes; do not
invent a target line count as a substitute for correctness or speed.

MI-005 [ ] Adapt the controlled benchmark to compare Eirene's ordinary output,
Tom-derived lightly fixed scalar sending and the same sender with OTIRX, using
identical bytes, submission boundaries, callback work and receiver checks.
Measure useful foreground work under pacing and maximum sustainable bulk rate.
Keep the previous larger candidate as a separately pinned reference where
useful; label cross-firmware comparisons clearly. Emulator timing alone is not
a physical performance result. Reuse the balls demo afterward for a visible
application comparison, rather than substituting it for controlled measurements.

## Explicitly deferred

Do not add multi-buffer queuing, copied RST18, completion event queues, raw
reserve/publish/cancel/reap APIs, elaborate ticket systems or diagnostic epochs
in the first version. They are not abandoned: reconsider each only when a
consumer or measurement justifies its cost. Preserve the old sources, qualified
binaries and reports without claiming their qualification transfers to Eirene.

Review the plan before beginning MI-001. No implementation has started on this
branch as part of this update.
