# INTEG-014 — Diagnose and reduce EMOS UART execution cost

Status: E04 authorised and in progress; E05 remains paused. No firmware implementation
started. Hardware notification and a review pause after each step.
Requested: 2026-09-13. EMOS owns implementation; Extender PORT-008 owns
paired transport/graphics qualification. This is the next UART priority.

## Objective and boundaries

Reduce the measured eZ80 transmit idle and receive backpressure while retaining
stock MOS compatibility and EMOS routing, keyboard, SD and resident-service
contracts. Establish comparable stock/EMOS throughput in both directions, first
without rendering, then with identical rendering work. Finishing a diagnostic
matrix is not equivalent to fixing the performance gap. Any remaining gap needs
an evidenced explanation and an explicit Author disposition before closure.

Use stock code where it fits. Otherwise make the smallest required adaptation.
Do not fix unrelated upstream bugs, redesign the wire protocol, change renderer
semantics, or resume parallel transport/Golem/game work under this task.
C/C++ is preferred when performance differences are negligible. Assembly is an
option for a demonstrated hot path or worthwhile flash saving, not a default
rewrite policy. Preserve upstream names, layout and reviewable control flow.

## Existing evidence and references

### Author clarification — reuse before optimisation

During planning the Author requested notes only. That pause was superseded by
the supervised-run instruction below. Audit redundant agent-written code before treating
the flash increase as an unavoidable feature cost or starting assembly rewrites.
“EMOS should be a wrapper around MOS, not more MOS” means a thin integration
layer reusing stock services inside the existing single EMOS firmware image;
it does not propose a second resident MOS or a new binary architecture.

Extend E03 beyond the UART hot paths to all EMOS-added functionality. Search
stock MOS thoroughly for existing implementations of each required operation:
command parsing/dispatch, file and SD access, memory/buffer management, timing,
serial I/O, packet handling, callbacks, formatting and error handling. Follow
internal stock routines as well as public APIs. For each added operation, record
the EMOS call sites, closest stock routine, whether it is already reused, and
the concrete reason any separate implementation remains necessary. An agent's
choice to write a new helper is not evidence that stock lacks the capability.

Produce a reuse table with linked byte costs and dispositions: reuse directly,
add a minimal adapter, retain with demonstrated semantic/ownership/ISR-context
requirements, or investigate further. Look for copied algorithms, parallel
utility layers, repeated conversions and redundant buffers/state. Distinguish
necessary per-UART state from duplicated logic: sharing stock parser globals
must not corrupt simultaneous packets. Check ABI, reentrancy, blocking and
interrupt assumptions before proposing a stock call as a replacement.

Prefer removing redundant work over translating that work to assembly. Measure
actual whole-image savings, including newly pulled-in helpers; do not claim
the sum of symbol sizes as guaranteed savings. Separate toolchain overhead,
diagnostic composition, unavoidable integration and demonstrated duplication.
The 21,581-byte image difference alone establishes neither redundancy nor agent
carelessness. Record evidence before attributing cause. Present reuse findings
before choosing the optimisation candidates in E05–E07.

1. Extender `docs/tasks/PORT-008/uart-alignment/FINDINGS.md`, `PLAN.md`,
   `PROCEDURE.md`, `WIRE-PROCEDURE.md` and `results/` retain the previous pass.
   Its bounded checklist is completed; overall transport parity is not.
2. Extender `docs/tasks/AUDIT-005/differences-and-reuse.md` and the accompanying
   baseline/path map already compare stock and EMOS. Refresh against actual
   source rather than repeating the audit or treating old conclusions as proof.
3. Read official MOS/VDP documentation and tagged reference sources read-only.
   Previous baseline: MOS 3.0.2, commit
   `8336409351ee5314e02801a7b72a4f1bb5282519`; VDP 2.16.0, commit
   `c7ac293d2aa81ddfa693390549bcd909069c8fc3`. Reverify before use.
4. Local contracts: [ownership](../../OWNERSHIP.md),
   [EMOS contract](../emos-v1-contract.md), existing INTEG-009/010/012 and
   BENCH-001 records. Generic translation/runtime changes belong in mos-agondev.

Previous wire measurements for aligned P4 code:

| Transfer | Observed interval | Wire occupancy | Relevant idle |
|---|---:|---:|---:|
| eZ80 → P4, 65,535 bytes | 2,099.394 ms | 568.880 ms | 1,530.514 ms with P4 permission; zero P4 backpressure |
| P4 → eZ80, 4,626 framed bytes | 109.609 ms | 40.132 ms | 69.336 ms with Agon stopping P4; 0.140 ms with permission |

These are diagnostic windows, not equivalent payload benchmarks. Captured bytes
were exact; digital evidence weighs against wiring as the dominant cause but
does not qualify all analogue margins. UART0 was not captured. Concurrent
ESP32 reception is a hypothesis, not an established explanation.

## Flash budget established during planning

| Composition/evidence | Flash window | Image occupied | Free |
|---|---:|---:|---:|
| Official stock MOS v3.0.2 release binary | 131,072 bytes | 108,490 bytes | **22,582 bytes** |
| Saved deployed BENCH-001 v0.1.16 image | 131,072 bytes | 130,071 bytes | **1,001 bytes** |
| Earlier ordinary composition, comment in `port/bench-telemetry.mk` | 131,072 bytes | Not independently remeasured this turn | **153 bytes recorded** |

Saved build: `agon-emos-v0.1.16-b2026-09-13-07-31-36Z`.
Binary SHA-256: `1da8330462eed54317e5889cf3f09cb1abaca8b478f7d92b88a3a1d74c9fc70c`.
Its map gives FLASH origin 0, length `0x20000`, and `__rom_image_end=0x1fc17`.
Binary length and manifest agree. Evidence is retained beneath the matching
build directory in Extender's ignored `agents/bench-001/` directory.
Manifest identifies source commit `af20d4ea2fd4b48b4a6e614a57c1cad8510341fc`
plus dirty source hashes. This is not a measurement of a fresh worktree build.
The bench profile substitutes telemetry for UARTFLOW; ordinary EMOS retains
UARTFLOW. Do not silently remove features to claim an optimisation. These are
flash figures, not available heap or stack RAM.

Follow-up stock check: the saved official `stock-mos-v3.0.2.bin` is 108,490
bytes, SHA-256 `d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae`.
Both match the official release asset size/digest in the retained
`stock-mos-v3.0.2.release.json` beside it under Extender's ignored
`agents/reference/emos-w1-2026-09-07/`. Stock `MOS.zdsproj` specifies ROM
`000000-01FFFF`. Thus unused trailing flash is 22,582 bytes (about 22.05 KiB,
17.23%). The bench EMOS image is 21,581 bytes larger. This compares shipped
stock and the saved bench image; it does not attribute the difference solely
to EMOS features, since toolchain and composition differences also matter.

## Execution checklist

### RAM clarification from planning discussion

MOS reserves external SRAM `0x0BC000–0x0BFFFF` (16,384 bytes) for static
state, heap and stack. The saved bench map places `__heapbot=0x0BDB34`:
6,964 bytes static allocation, 7,372 bytes heap arena up to `0x0BF800`, and
2,048 bytes reserved stack to `0x0C0000`. `src/defines.h` defines the 2,048-byte
stack reserve and `main.c` passes HEAP_LEN to the allocator. Heap arena capacity
is not live free heap: allocator metadata and runtime allocations consume it.

This is external RAM, despite misleading “internal RAM” wording in linker
assertions and older qualification notes. The separate on-chip fast SRAM is
8,192 bytes at `0xB7E000–0xB7FFFF`; stock `src/mos.c` reports it as USER:HI.
Do not assume that region is available for exclusive EMOS use. Official MOS
documentation also marks `0x0B8000–0x0BBFFF` reserved, but allows applications
to use it; it is not spare space in this linked heap. Audit ownership and
compatibility before considering either region. Source project RAM bounds
resolve the extra trailing F typo in the documentation's heap/stack range.

Author clarification: the 8 KiB on-chip SRAM has been used by ordinary
applications and is not reserved for MOS. The Author reports no practical
access-time advantage over external SRAM in this setup; do not assume that
moving code there is a speed optimisation. Timing claims require separate
verification if they become relevant.

An emergency option discussed, not selected or authorised for implementation,
is bootstrapping EMOS extensions, TSRs or interrupt handlers into that SRAM,
probably from the SD card. Aware applications could coexist under an explicit
memory-ownership contract. Unaware applications may already use those addresses;
silently occupying them would compromise the promised near backward
compatibility in ExCom mode. Do not count these 8 KiB as available EMOS budget
or infer application consent from ExCom selection. Loading code from SD changes
its storage source, not the RAM ownership conflict.

Prioritise eliminating duplicated functionality and reducing costs within the
existing allocation. Revisit this fallback only with demonstrated need and an
Author-reviewed contract covering application admission, handler lifetime,
load/unload and recovery. That fallback design remains unapproved.

Commit each completed implementation-sized step with evidence and a rollback
point. Review this checklist before every step. Amend it only for recorded new
findings or Author direction. The Author authorised a supervised run on
2026-09-13 local time (2026-09-14 UTC). Complete one numbered step, notify on
hardware, then stop for the Author's response before beginning the next step.
This replaces the unattended cadence; it does not expand scope. Human
validation gates still apply to commits
of emulator-coupled changes; do not push experimental code without review.

1. [x] **E01 — Freeze inputs and recovery.** Preserve existing dirty EMOS source,
   tests, telemetry profile and TODO changes without mixing or discarding them.
   Record source hashes, toolchain/profile, stock tags and all deployed images.
   Read Extender's local hardware record and bench constraints. Verify rollback,
   SD service and keyboard access before firmware experiments. Freeze this plan
   before code work. Do not infer current physical state from an old manifest.
2. [x] **E02 — Reproduce both compositions and budget.** Build ordinary and bench
   EMOS using repository wrappers or generic repository-root EMOS-profile
   targets, with every FIRMWARE_LINK_CHECK including the UART divisor guard.
   Retain maps, ELF, disassembly and hashes. Account for text, constants, data
   initialisers, linker gaps, helper routines, static RAM and stack/heap margin.
   Compare stock ZDS and stock AgonDev outputs where available to separate
   compiler/translation overhead from added functionality; label missing outputs.
3. [x] **E03 — Refresh stock comparison.** Produce a side-by-side source and
   linked-instruction table for counted VDU writes, UART TX/CTS waits, UART0/1
   IRQ entry/exit, RX drain, packet parsing/dispatch and clock/deadline work.
   Trace `EMOS_vdu_WRITE` → `emos_console_write_stream` →
   `emos_keyboard_send` → per-byte `transmit`/`deadline_step`/
   `uart1_keyboard_put`. Charge the outer bridge once per block, not per byte.
   Identify required EMOS work separately from accidental duplication or
   compiler overhead. Record costs of C helpers and register saves, not guesses
   based on source language. Rank candidates by measured cost and bytes.
4. [ ] **E04 — Establish controlled baseline.** Reuse existing pure-data fixtures
   and exact-byte validation in forward, reverse and mixed directions. Lock one
   verified aligned P4 image throughout EMOS comparisons: the previous pass
   restored pre-test P4 firmware, so deployment identity matters. Keep stock VDP,
   baud, payload, framing, modes and wiring fixed. Start with browser/video output
   off. Repeat paired runs to establish variance; retain host, MOS-clock and
   wire intervals separately. Freeze a numerical material-improvement threshold
   from baseline noise before comparing language variants.
5. [ ] **E05 — Isolate transmit cost.** Distinguish C call/deadline/locking cost,
   CTS/THR polling and interrupt occupancy. Compare the existing path with the
   smallest stock-shaped C implementation; if still justified, compare a bounded
   assembly inner loop with identical contracts. Keep completion, partial-write
   errors and ownership semantics identical. One causal change per candidate.
6. [ ] **E06 — Isolate receive and dual-UART cost.** Measure IRQ frequency, entry
   saves, FIFO drain/packet work and RTS duration. Compare UART0 quiet, ordinary
   service traffic and controlled traffic while UART1 transfers run, then the
   converse where supported. Test-only gating must be separately labelled and
   restored; disabling VBlank, clock, keyboard or required replies is not a
   production fix. Compare stock assembly receiver structure with current C
   output and a minimal alternative only where evidence justifies it. Obtain
   additional UART0 capture channels only if needed; never infer unobserved
   signals or change wiring from legacy assumptions.
7. [ ] **E07 — Select and implement minimal fixes.** Choose C/C++ when timing is
   within the frozen negligible-difference threshold. Select assembly only for
   demonstrated throughput or meaningful linked flash savings, documenting
   bytes saved, speed, ABI burden and readability tradeoff. Keep independent
   TX, RX and space changes in separate commits. If a broader rewrite appears
   necessary, present the measured size ledger and bounded proposal first.
8. [ ] **E08 — Preserve correctness under stress.** Test simultaneous packet
   streams with independent parser state, required callback/register/sysvar
   behavior, routing/leases, source switching, interrupted and partial packets,
   stale RX/errors, CTS stalls, stopped-clock bounded waits and IRQ-off callers.
   Preserve active-low GPIO CTS versus stock inverted modem-status conventions.
   Preserve bounded receive work, timeout and recovery contracts; copying a
   stock unbounded wait is not automatically compatible. Run meaningful host
   and linked checks, then physical exact-byte/mixed-load regression tests.
9. [ ] **E09 — Add rendering load.** Repeat the frozen paired graphics suite
   after pure transport passes. Keep initial P4 output off; separate upload,
   resident bitmap/sprite draw and output timing. Preserve known probe differences
   and the earlier mainboard timeout as unresolved evidence; do not hide them by
   editing tests or extending timeouts. Verify ordinary CLI, SD and keyboard
   recovery. Show a concise end-of-test summary on new fixtures where appropriate.
10. [ ] **E10 — Report, restore and review.** Report worst-first tables in ms:
    stock baseline, original EMOS, final EMOS, `(candidate/stock - 1) × 100%`
    elapsed-time difference, variance, bytes/s and correctness. Include flash
    and RAM ledgers for both compositions. Remove diagnostic overhead for the
    production candidate and remeasure. Restore agreed firmware/startup state,
    neutral keys and working CLI/SD. Obtain human visual/interactive acceptance;
    historical or emulator success does not validate a new hardware build.
    Close only with performance/correctness evidence and Author disposition of
    residual gaps. Publication remains a separate reviewed action.

## Decision register

1. Accepted Author preference: C/C++ for negligible performance differences;
   targeted assembly may win on speed or constrained flash space.
2. Pending measurement: negligible-difference threshold and required flash
   reserve. Establish from repeatability and actual release composition before
   candidate selection; merely fitting below 128 KiB is not a growth budget.
3. Pending evidence: whether dual-UART servicing materially causes the gap.
   Do not change production receive/clock contracts to make a benchmark pass.

## Planning checkpoint

E01 [preservation/recovery](INTEG-014/E01.md) and E02
[reproduction/accounting](INTEG-014/E02.md), plus E03
[stock-reuse/execution audit](INTEG-014/E03.md), are complete. Ordinary EMOS has
157 flash bytes free; the earlier 153-byte comment is superseded by measured
binary/map evidence. Wait for the Author before E04. Existing dirty EMOS work
is preserved; source optimisation starts only at its later step.

E04 approval on 2026-09-14: [bounded baseline procedure](INTEG-014/E04-procedure.md)
freezes inputs, repetition count, recovery and threshold rule before deployment.
