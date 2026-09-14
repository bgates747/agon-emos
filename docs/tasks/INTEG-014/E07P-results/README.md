# E07P UART parity measurements — in progress

## Executive summary

The latest forward candidate is faster than the mainboard in four paired
observations: **589.036 versus 589.663 ms**. It passes all 376
original exact controls. Final six-sample/three-capture qualification remains.
The latest matched reverse measurement still shows a 5.49% gap; the next receive
guard is being tested. The [frozen task contract](../E07-parity.md) owns execution.

Latest evidence by workload, ranked by percentage gap. These rows use the
explicitly named intermediate images; they are not a joint final qualification.
Negative means Extender takes less elapsed time.

| Workload and candidate | Mainboard baseline (ms) | Extender (ms) | Difference |
|---|---:|---:|---:|
| Return, per 256-packet transfer in 16-transfer batch — RX02 | 85.417 | 90.104 | +5.49% |
| Forward 65,535 pseudorandom bytes — TX04 | 589.663 | 589.036 | -0.11% |

Forward combines three full-matrix samples and one independent wire-run endpoint
result. Return uses six paired intervals with identical request, arming and
mailbox-copy work; each per-transfer bound is ±1.042 ms. That batch verifies
393,216 useful bytes exactly. These are transport measurements, not graphics or
output timings. [Smaller transfers](rx02-lengths.json) still expose setup latency;
the separate P4 owner-scheduling comparison targets a stock-loop divergence.

| Candidate | Physical disposition | Evidence |
|---|---|---|
| TX01 fused block | Retained: 591.042 ms forward median; 376 original exact cases pass | [TX01](tx01.json) |
| RX01 parser | Retained: 109.608→95.528 ms UART1 return wire; 376 cases pass | [RX01](rx01.json) |
| TX02 short branches | Rejected: two repeatable forward regressions; 8 exact cases pass | [TX02](tx02.json) |
| RX02 FIFO drain | Retained: 95.530→87.718 ms return wire; 376 original cases pass | [RX02](rx02.json), [matched return](ep2rb1-analysis.json) |
| TX03 status classification | Retained: 590.064 ms forward median; 376 cases pass | [TX03](tx03.json) |
| TX04 original branches restored | Retained: 589.036 ms forward median; 376 cases pass; final qualification pending | [TX04](tx04.json) |

Wire measurements in that table are UART1-only observations, not fabricated
UART0 wire comparisons. The mainboard's individual blocking-handler timer and
P4 enqueue timer are deliberately excluded from matched reverse claims.

Immutable firmware/source manifests and raw logic captures remain in the ignored
E07P evidence directories in EMOS and Extender. Each JSON pins image/capture
hashes and each CSV retains its terminal integrity/recovery result. No final
parity qualification or restoration has been claimed yet.
