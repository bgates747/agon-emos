# E07P UART parity qualification

## Executive summary

Both frozen bulk-transfer targets pass on physical hardware in **both ordinary
and bench EMOS profiles**. Ordinary EMOS takes **588.096 ms versus mainboard
589.670 ms forward**, and **85.156 ms versus 86.068 ms return**. Each profile
passes 384 original exact/mixed/wire cases and three independent captures.
The longer symmetric return test separates the timing bounds. The exact pre-run bench
firmware and startup are restored and verified; the final attention cue is
pending. Nothing was pushed.

This qualifies the specified transport workloads, not every short command or
graphics operation. Short-command setup still adds about 0.24–0.33 ms. The
[frozen contract](../E07-parity.md) owns scope, decisions and execution gates.

## Paired transport results

Ordinary profile, worst percentage gap first; negative means the P4 route takes
less elapsed time. Both reverse routes terminate at the same eZ80 application.

| Workload | Mainboard VDP route (ms) | P4 EDP route (ms) | Elapsed difference |
|---|---:|---:|---:|
| 65,535 random bytes forward | 589.670 | 588.096 | -0.27% |
| 2,048 useful bytes returned in 256 packets | 86.068 | 85.156 | -1.06% |
| 65,535 forward bytes during return traffic | 672.608 | 643.330 | -4.35% |

Forward has six selected observations per profile. Ordinary ranges are
589.635–589.760 ms mainboard and 588.054–588.149 ms P4. Mixed forward uses three
full-matrix observations. Return uses six alternating paired intervals of 128
identical transfers, including equal request/arming/mailbox-copy work; validation,
SD writes and screen output occur outside each timed interval. Each profile
verifies 3,145,728 useful bytes in that long test, plus 393,216 in its preserved
16-transfer control. The short controls had overlapping bounds and were not
called parity passes.

At the measured two clock units per 60 Hz VBlank, conservative per-transfer
uncertainty is ±0.130208 ms for the long interval. Ordinary P4 upper bound
85.286 ms is below mainboard lower 85.938 ms. This avoids comparing P4 enqueue
time with mainboard blocking-handler time. No UART0 wire capture is claimed.
[Ordinary evidence](rx05-ordinary.json), [long result](ep5olong-analysis.json),
[bench evidence](rx05.json), [full comparison and bytes/s](qualification.json).

## Observed profile difference

P4 route medians, ordinary as baseline. These are profile differences: the
bench composition substitutes telemetry for UARTFLOW, so they are not an
isolated measurement of instrumentation alone.

| Workload | Ordinary (ms) | Bench (ms) | Bench excess |
|---|---:|---:|---:|
| Mixed forward | 643.330 | 655.773 | +1.934% |
| Return | 85.156 | 85.286 | +0.153% |
| Forward | 588.096 | 588.154 | +0.010% |

All six wire captures decode independently, match exact bytes, preserve recovery
and report zero browser snapshots. Median UART1 return wire spans are 83.837 ms
ordinary and 83.952 ms bench; these exclude the end-to-end work above.

## Remaining short-command latency

Ordinary full-matrix random-payload medians include the unchanged READY/query
sequence. This is a remaining fixed-cost limitation, not a claim of all-size
latency parity.

| Forward payload | Mainboard (ms) | P4 (ms) | Difference |
|---|---:|---:|---:|
| 0 bytes | 0.440 | 0.681 | +54.77% |
| 256 bytes | 2.793 | 3.119 | +11.67% |
| 4096 bytes | 37.299 | 37.539 | +0.64% |

## Implementation and qualification boundaries

EMOS now keeps the bounded transmit block in registers, uses a stock-shaped
byte parser/FIFO drain with C dispatch for complete packets, and avoids two
proved private-call costs. The P4 owner no longer adds an unconditional 1 ms
sleep absent from stock VDP's hardware loop. Baud, wire protocol, renderer,
required clock/keyboard/SD services, five-second deadlines, stopped-clock
termination, partial results and full IRQ register protection are preserved.
No wiring change or on-chip user-SRAM reservation was needed.

Both canonical build profiles and all 91 host tests pass. Linked instruction
checks cover 8,683,703 public parser bytes, 8,582,356 caller-admitted private
bytes, 3,906 bench/1,302 ordinary IRQ cases and 375 complete sender cases. The
P4 owner/stream controls pass ASan/UBSan, including deliberate failure controls.
The 128-transfer fixture retains every byte and leaves 178,210 application RAM
bytes below its stack top; its rebuilt default-16 binary matches the original
after replacing only its identity. These tests do not validate new gameplay,
every interrupted CPU mode, graphics timing or analogue wiring integrity.

## Linked resource budgets

All figures are bytes; flash capacity is 131,072 and the existing MOS external
SRAM reservation is 16,384. Heap precedes allocator metadata/live allocations;
stack reserve is not a measured high-water mark. [Map hashes](budgets.json).

| Profile | Flash used | Flash free | Static SRAM | Heap arena | Stack reserve |
|---|---:|---:|---:|---:|---:|
| Ordinary | 130,936 | 136 | 6,826 | 7,510 | 2,048 |
| Bench | 130,091 | 981 | 6,964 | 7,372 | 2,048 |

## Candidate history

The pre-run P4 forward value was 1,070.062 ms; the final ordinary value is
588.096 ms, a 45.04% reduction in elapsed time. Intermediate results below are
explicitly named compositions, not a pooled final measurement.

| Candidate | Physical disposition | Evidence |
|---|---|---|
| TX01 fused block | Retained: 591.042 ms forward median; 376 original exact cases pass | [TX01](tx01.json) |
| RX01 parser | Retained: 109.608→95.528 ms UART1 return wire; 376 cases pass | [RX01](rx01.json) |
| TX02 short branches | Rejected: two repeatable forward regressions; 8 exact cases pass | [TX02](tx02.json) |
| RX02 FIFO drain | Retained: 95.530→87.718 ms return wire; 376 original cases pass | [RX02](rx02.json), [matched return](ep2rb1-analysis.json) |
| TX03 status classification | Retained: 590.064 ms forward median; 376 cases pass | [TX03](tx03.json) |
| TX04 original branches restored | Retained: 589.036 ms forward median; 376 cases pass; final qualification pending | [TX04](tx04.json) |
| RX03 idle telemetry guard | Rejected: no demonstrated wire gain | [RX03](rx03.json) |
| RX04 register argument | Retained receiver candidate: 85.960 ms return wire; 376 cases pass, combined parity still open | [RX04](rx04.json) |
| P4 owner sleep removed | 588.147 ms forward; 376 cases and service/idle checks pass; retained | [Owner comparison](owner01.json) |
| RX05 caller-proven private fault admission | Retained: 83.952ms return wire, strict matched reverse parity; 384 exact cases | [RX05](rx05.json) |

Immutable source/firmware manifests, ROM readbacks and raw logic traces remain
in the ignored E07P evidence directories in EMOS and Extender. Result JSON
pins image/capture hashes, and CSVs retain terminal integrity/recovery records.
The [restoration record](restoration.json) verifies the full pre-run EMOS ROM,
affected mainboard VDP sectors and exact P4 prefix, unchanged startup/backup,
257-byte SD roundtrip, actual keyboard-launched CLI COPY, and neutral input.
All129 original generic-port source files and136 generated files are preserved
or restored byte-exact, with current outputs retained separately. Qualified
images remain available; the bench deliberately runs its original working
firmware for review. The hardware voice cue is the final pending step.
