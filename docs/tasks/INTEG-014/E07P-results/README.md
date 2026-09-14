# E07P UART parity measurements — in progress

## Executive summary

RX04 with the P4's extra owner-loop sleep removed is faster than mainboard for
large forward transfers: **588.147 versus589.595ms** across four paired samples.
All376 original exact/mixed/wire cases pass. The symmetric return benchmark
still misses parity: **87.500 versus85.417ms** (+2.44%). The follow-up RX05
receiver is prepared and instruction-tested; it has not been deployed. Final
six-sample/three-capture qualification and original bench restoration remain.
The [frozen task](../E07-parity.md) owns execution.

Current paired composition, worst percentage gap first. Negative means P4
takes less time; these are transport measurements, not rendering/output.

| Workload | Mainboard baseline (ms) | Extender (ms) | Difference |
|---|---:|---:|---:|
| Return, per256-packet transfer in16-transfer batch | 85.417 | 87.500 | +2.44% |
| Forward65,535 pseudorandom bytes | 589.595 | 588.147 | -0.25% |

Forward combines three full-matrix samples and one wire-run endpoint result.
Return uses six paired intervals with equal request/arming/mailbox-copy work,
393,216 exact useful bytes and conservative ±1.042ms per-transfer bounds. It
does not compare P4 enqueue time with mainboard blocking-handler time.
[Batch evidence](epob1-analysis.json).

Short-command latency also improved after removing the sleep, but is not equal
yet. These full-matrix medians include the unchanged READY/query sequence.

| Forward random payload | Mainboard (ms) | P4 (ms) | Difference |
|---|---:|---:|---:|
| 0 bytes | 0.439 | 0.693 | +57.86% |
| 256 bytes | 2.803 | 3.131 | +11.70% |
| 4,096 bytes | 37.329 | 37.553 | +0.60% |

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

Wire measurements in that table are UART1-only observations, not fabricated
UART0 wire comparisons. The mainboard's individual blocking-handler timer and
P4 enqueue timer are deliberately excluded from matched reverse claims.

Immutable firmware/source manifests and raw logic captures remain in the ignored
E07P evidence directories in EMOS and Extender. Each JSON pins image/capture
hashes and each CSV retains its terminal integrity/recovery result. No final
parity qualification or restoration has been claimed yet.
