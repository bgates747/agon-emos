# TR-001 Console8 results: R0054–R0059

The smaller sender keeps the performance benefit while removing about 40% of
its MOS instructions. In this controlled Console8 run, it allowed 64.6% more
foreground calculation than official stock MOS at an audio-like paced sending
rate, and retained 86.0% of the no-traffic calculation rate while sending flat
out. Bulk throughput remained essentially stock. Compared with published RC1,
the main gain is less code; differences in measured speed/work were small.
This supports the trimmed version as the next application-test candidate.
It has not been promoted or published as a new release.

All six expected logged runs passed with exact matching ROM and app
identities. The 3 performance reports contain 27 complete trials; RC1 regression
and both trial regressions passed. All 245760 payload bytes were verified, with
no missed pacing deadlines. Independent-clock checks, calibration/checksums,
callback counts, journal START/PASS entries and sequential counter passed.
No deployed input was modified; no firmware or fixture was changed during
inspection. The next report number is60.

| Measurement (median, 3 repeats) | Official stock MOS3.0.2 | Published RC1 | Smaller TRIM Trial1 |
| --- | ---: | ---: | ---: |
| Bulk-only payload, decimal kB/s |106.255|106.110|106.204|
| Bulk+calculation payload, decimal kB/s |106.251|106.142|106.246|
| Paced useful work, steps/s |7082.51|11618.26|11658.97|
| Paced useful work increase over stock |baseline|64.04%|64.62%|
| Bulk useful work, steps/s |0|11583.36|11646.71|
| Bulk useful work retained vs no-traffic calibration |0%|85.52%|85.99%|

Paced payload averaged approximately 44.1 kB/s by design; it is not the bulk
throughput figure. One step is one identical xorshift calculation with checked
state, not an exclusive CPU-utilization measurement. Baseline no-traffic
calibration was identical across all 3 MOSes: 2517 steps in 214075 PRT ticks, with
state 75D9D8D6. The compiled C measurement and timer objects match across these
apps. Each trial sends 8192 payload bytes within 8448 wire bytes.

Relative to RC1, TRIM completed 0.350% more paced work and 0.547% more bulk work;
bulk-only throughput differed by 0.089%. Three short samples, sequential MOS
installation and a single board do not justify claiming a material sustained
speedup. The defensible result is preserved performance with a smaller sender.
Older receiver-fixed VDP measurements remain separate; this matched series
used the agreed stock Bistromathics 2.16.0 protocol. VDP identity and the
nonlogging example/cold-boot checks rely on the Author's overall completion
report, not an independent capture. Nothing is missing from the logged protocol.

| Report | Application / installed MOS | Outcome |
| --- | --- | --- |
|R0054|stock / official stock CRC15BD0583|PERF2 PASS,9trials|
|R0055|checkrc / published RC1 CRC3F792F8D|RC1 PASS|
|R0056|rc1 / published RC1 CRC3F792F8D|PERF2 PASS,9trials|
|R0057|test / trial CRCD5CC99AE|TRIM1 PASS|
|R0058|test / trial CRCD5CC99AE|TRIM1 PASS|
|R0059|trim / trial CRCD5CC99AE|PERF2 PASS,9trials|

The ROM still present at the last logged trial was TRIM Trial1; its source is
df2066832ecb6a6a33a817acb8b9f41d609e44cc, 100958 image bytes, sender 286 instruction
bytes and 9 RAM bytes. Published RC1 uses 476 instruction / 11 RAM bytes; total MOS
image difference is 182 bytes because trial branding adds 8. The trial's C app
gate costs 29 bytes; the matched benchmark's whole ABI bridge grows by 41 bytes.
Caller context/ownership preconditions and raw-callback obligations remain the
tradeoff documented in the [trial contract](../../../experiments/uart0-trim/contract.md); passed tests do not reinstate removed checks.

Verified full card archive: build/tm001/tr001-hardware001/returned.tar.gz,
SHA256 89d1e0ab7e09c948e75c722b5098c6bb502a4e34f71b07f057f1b557fd5635bb.
It and all returned files are checksum-verified on both Oryx and Pi. Card volume
was AGONC8, UUID8658-1C23. Only host archive/analysis files were written; the card
was not modified, remounted or ejected. Raw reports are in hardware/; full
measurements and validation are in hardware.json, with archive receipt in
hardware-receipt.json. The host summary now correctly distinguishes functional
reports from historical PERF1 reports; numeric measurement formulas are unchanged.
