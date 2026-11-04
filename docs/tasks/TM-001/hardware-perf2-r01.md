# Five-case bulk calculation results on Console8

All five requested fixtures ran correctly. R0047..R0051 each identify the
expected app build and matching installed-MOS prefix CRC. All45timed trials
passed,368640receiver payload bytes were verified, all work-state checksums
and callback counts matched, both PRT counters agreed and no paced completion
deadline miss was recorded. Each journal START has its PASS; next.txt is52.

The new result is that the small Eirene OTIRX sender allows substantial
foreground calculation even during flat-out sending. It delivers essentially
the same payload rate as stock in the bulk-plus-work scenario while completing
calculation at85.5% of its no-traffic calculation rate. Stock and unmodified
Eirene complete no foreground calculation inside that bulk transfer because
their sending calls block the application. That is an API scheduling result,
not evidence that their CPU physically has no idle cycles or that separate
application interrupt handlers could do no work.

## Matched results

Medians of three samples per scenario per firmware. kB/s means1000payload
bytes per second, excluding command headers. Calculation percentages compare
completed work rates, not CPU utilization. Paced gain uses official stock MOS.

| Firmware / path | Bulk only kB/s | Bulk + work kB/s | Bulk work as % of no-traffic rate | Paced work gain over stock |
| --- | ---: | ---: | ---: | ---: |
| Official stock MOS 3.0.2 | 105.23 | 106.20 | 0.0% | baseline |
| Unmodified Eirene | 107.41 | 107.41 | 0.0% | -0.02% |
| Minimal Eirene scalar async | 104.76 | 106.13 | 79.3% | +59.11% |
| Minimal Eirene OTIRX async | 104.85 | 106.14 | 85.5% | +64.06% |
| Previous full OTIRX borrowed/direct | 98.32 | 97.83 | 73.1% | +50.70% |

Minimal OTIRX completes7.86% more bulk calculation than minimal scalar at
essentially identical bulk-plus-work throughput. In paced sending its
calculation advantage over scalar is3.11%. Compared with the previous full
implementation, minimal OTIRX completes17.00% more bulk calculation and has
higher throughput. The minimal OTIRX bulk-plus-work rate is0.063% below the
stock bulk-plus-work rate, far too small a gap here to claim a useful transport
difference. Its8448wire bytes per8192payload bytes imply about109453wireB/s,
95.0% of the115200wireB/s theoretical ceiling for1.152Mbaud8N1.

Absolute credited calculation rates in bulk-plus-work are0steps/s for stock
and pristine Eirene,10739for scalar,11583for minimal OTIRX and9900for the
previous full implementation. Their corresponding paced rates are7082,7081,
11268,11618and10672steps/s. All no-traffic calibrations completed2517steps.
These are PERF2 single-xorshift work units; do not compare their raw counts
with PERF1 units of sixteen steps.

## Interpretation and limits

The most promising choice remains the small Eirene OTIRX candidate. Its main
benefit is overlapping useful application work with transmission, including
when sending flat out; a bulk-speed improvement over stock is not established.
The scalar alternative remains close in throughput but yields less calculation
capacity. Preserve the fuller implementation as comparison evidence rather
than adding its complexity back without a demonstrated application need.

The bulk-only medians can be slightly slower than the corresponding bulk-work
medians. The three samples are short8KiB payloads and vary with scheduling;
do not infer that adding calculation causes a throughput improvement. Rotating
scenario order within each firmware does not randomize firmware order or
establish statistical precision or indefinite sustained rates. The last
potentially straddling bulk work unit is discarded from credited counts.
Observation time includes the final work quantum and completion-check cost.

Paced transmission uses256-byte payload chunks averaging44100payloadB/s,
not44100single-byte application interrupts per second. This is not a real-app
audio-latency qualification, EMOS fit test, or an electrical CTS-margin result.
No-traffic calibration uses the same work kernel on each firmware. Both PRTs
share the system oscillator; their agreement checks accounting rather than
independently calibrating its absolute frequency.

## Integrity and custody

The identified card is AGONC8 UUID8658-1C23. Every immutable file from the
verified deployment still matches, including all five apps/MOS files, VDP
images, comparator and previous/ contents. Only R0047..R0051 appeared and the
normal runs.log/next.txt changed. Historical R0042..R0046 are byte-identical.
Agon file timestamps are unreliable and were ignored. The card was read only;
no firmware was flashed or card file changed during inspection.

The reports establish the last tested installed MOS was the previous full
OTIRX image. They do not independently fingerprint the installed VDP. The
matched receiver-fixed VDP was required by the protocol and remains staged;
a restoration step was not required while Console8 is dedicated to UART tests.

[Raw returned reports and journal](hardware-perf2-r01/).
[Full sample data, identities and custody hashes](hardware-perf2-r01.json).
A verified complete appliance archive is retained on Pi and Oryx at
`build/tm001/hardware-perf2-r01/returned.tar.gz`, SHA256
6732900ddf826e6f17b4549dfd52394845363f8faac0147695a38e1da41ab89c.
The extracted appliance, receipt and machine-readable summary are beside it.

BW-005 is complete. The next application step is adapting the existing balls
demo to the small Eirene single-buffer syscall, keeping stock output as its
baseline and preserving the same rendering/physics work. That will test the
gain in a visible real workload.
