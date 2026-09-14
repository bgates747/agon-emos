# E07P UART parity measurements — in progress

## Executive summary

RX05 with the retained P4 owner-loop alignment has passing candidate results
in both directions. All376 original exact/mixed/wire cases pass; final captures
and ordinary-profile qualification remain. The128-transfer reverse test
verifies3MiB exactly and separates the timing bounds. Original bench restoration
is still pending. The [frozen task](../E07-parity.md) owns execution.

Current paired composition. Negative means P4 takes less elapsed time; these
are transport measurements, not rendering/output.

| Workload | Mainboard baseline (ms) | Extender (ms) | Difference |
|---|---:|---:|---:|
| Forward65,535 pseudorandom bytes, four observations | 589.690 | 588.142 | -0.26% |
| Return, per256-packet transfer in128-transfer batch | 86.068 | 85.286 | -0.91% |

Return uses six alternating paired intervals with equal request/arming/copy
work and3,145,728 exact useful bytes. Conservative per-transfer bounds are
±0.130208ms: P4 upper85.417ms is below mainboard lower85.938ms. It does not
compare P4 enqueue time with mainboard blocking-handler time.
[Long result](ep5rlong-analysis.json); the preserved [short control](ep5rb1-analysis.json)
had overlapping bounds and was not called a pass.

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
| RX05 caller-proven private fault admission | Retained: 83.952ms return wire, strict matched reverse parity; 376 exact cases | [RX05](rx05.json) |

Wire measurements in that table are UART1-only observations, not fabricated
UART0 wire comparisons. The mainboard's individual blocking-handler timer and
P4 enqueue timer are deliberately excluded from matched reverse claims.

Immutable firmware/source manifests and raw logic captures remain in the ignored
E07P evidence directories in EMOS and Extender. Each JSON pins image/capture
hashes and each CSV retains its terminal integrity/recovery result. No final
ordinary-profile qualification or restoration has been claimed yet.
