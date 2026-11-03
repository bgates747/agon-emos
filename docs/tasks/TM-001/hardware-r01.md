# Console8 comparison: all requested runs complete

Nothing was missed in the requested three-image comparison. R0042 tested
ordinary pristine Eirene output, R0043 the lightly fixed Tom-derived scalar
sender, and R0044 its OTIRX version. All three installed MOS prefix CRCs match
the pinned images. All reports contain PERF1 PASS and their journals close PASS.

The small interrupt-driven sender frees substantial foreground calculation time
while sending paced traffic. OTIRX improves that headroom a little further.
Neither async variant demonstrated higher bulk throughput in these short tests.
That supports trying the small OTIRX candidate in an actual application; it
does not establish release approval or EMOS compatibility.

## Physical results

Medians of three samples per workload per firmware, measured with the fixture's
PRT counters. Rates below are decimal bytes/second, not baud. Paced traffic is
256-payload-byte chunks averaging44100payload bytes/second, plus command headers.

| Sender | Bulk payload B/s | Bulk wire B/s | Bulk change | Paced calculation units/s | Calculation gain |
|---|---:|---:|---:|---:|---:|
| Pristine Eirene ordinary output |107373|110728|baseline|648.9|baseline|
| Lightly fixed Tom-derived scalar |105350|108643|-1.9%|1035.5|+59.6%|
| Same sender with OTIRX |106062|109376|-1.2%|1067.9|+64.6%|

OTIRX completed about3.1% more foreground calculation than scalar. This is
**more useful work in a matched observation window**, not a claim that CPU
usage fell64.6%. Each calculation unit executes the same16-step xorshift kernel;
its final checksum was independently validated. Submission-wrapper time excludes
interrupt work and must not be presented as total sender CPU consumption.

All18timed trials passed, with147456received payload bytes verified. Negative
and positive comparator controls passed in each run, expected callback counts
matched, both timer counters agreed and there were no recorded paced completion
deadline misses. Callback work was identical between scalar and OTIRX. The last
source-completion observations are distinct from local wire drain; the separately
recorded remote GP fence is excluded from bulk throughput.

The three bulk tick ranges overlap: ordinary87860..89620, scalar89023..90165,
OTIRX88942..89129. With only three8KiB samples per variant, the small differences
do not justify a confident bulk ranking. The paced work counts were stable:
121units for ordinary output,193for scalar and199for OTIRX in each of three runs.
This does not test44100single-byte IRQs/second, audio latency, indefinite bulk
load, collision/graphics workloads or physical CTS margin.

## Integrity, firmware state and custody

Fixture build1ad95f651c6e017d and test.bin SHA256
0f817a364acd229d48fac395f81aec486a7f64963523d6ee4b4d01cc14583490.
Sources, image pins and protocol: performance-r02.md and performance.md.
All immutable deployed files match the deployment receipt, including the
firmware, application, comparator, licenses and provenance. next.txt is45;
runs.log contains exactly the expected START/PASS pairs for R0042..R0044.
Unreliable Agon file dates were ignored; report numbering establishes order.

The last run proves the OTIRX experimental MOS was installed then (CRC32
B201C2A4). No subsequent restoration was reported or requested. The matched
receiver-fixed VDP image is intact on the card and was requested by the protocol;
this report format does not independently identify the installed VDP. Keeping
the experimental pair between dedicated UART tests is already authorized;
restoration was not a missing required step in this round.

Raw returned appliance and parsed analysis are archived on Pi and Oryx at
`build/tm001/hardware-r01`. Inspection did not write the SD card. The JSON
companion contains the complete samples, checksums, comparison and limitations.
Pi archive: `build/tm001/hardware-r01.tar.gz`, SHA256
594d50d820858b98ec3d5689b175780ddc9ca15a12dc12ee1a57a2239b2be6e6.

TM-005 and deployment DP-004 are complete. The next planned application step
is adapting the existing balls demonstration to Eirene's explicit42h/43h
single-buffer interface, preserving this controlled benchmark as the reference.
