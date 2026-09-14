# E07P UART parity measurements — in progress

## Executive summary

Both directions are substantially faster, but parity is still unmet. The latest
fully checked receiver candidate takes 5.49% more time for matched return work
and0.42% more for the large forward upload. Further isolated candidates
are in progress under the [frozen task contract](../E07-parity.md).

Ranked by percentage gap; negative would mean Extender takes less elapsed time.

| Matched workload | Mainboard baseline (ms) | Extender (ms) | Difference |
|---|---:|---:|---:|
| Return, per 256-packet transfer in 16-transfer batch | 85.417 | 90.104 | +5.49% |
| Forward 65,535 pseudorandom bytes | 589.594 | 592.053 | +0.42% |

Forward values combine three full-matrix samples and one independent wire-run
endpoint result. Return uses six paired intervals with identical request,
arming and mailbox-copy work; each per-transfer bound is ±1.042 ms. The batch
verifies 393216 useful bytes exactly. These are not graphics or output timings.

| Candidate | Physical disposition | Evidence |
|---|---|---|
| TX01 fused block | Retained:591.042 ms forward median;376original exact cases pass | [TX01](tx01.json) |
| RX01 parser | Retained:109.608→95.528 ms UART1 return wire;376 cases pass | [RX01](rx01.json) |
| TX02 short branches | Rejected:two repeatable forward regressions;8exact cases pass | [TX02](tx02.json) |
| RX02 FIFO drain | Retained:95.530→87.718 ms return wire;376original cases pass | [RX02](rx02.json), [matched return](ep2rb1-analysis.json) |

Wire measurements in that table are UART1-only observations, not fabricated
UART0 wire comparisons. The mainboard's individual blocking-handler timer and
P4 enqueue timer are deliberately excluded from matched reverse claims.

Immutable firmware/source manifests and raw logic captures remain in the ignored
E07P evidence directories in EMOS and Extender. Each JSON pins image/capture
hashes and each CSV retains its terminal integrity/recovery result. No final
parity qualification or restoration has been claimed yet.
