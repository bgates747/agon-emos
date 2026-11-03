# Complete five-case Console8 comparison

All requested cases are complete. R0045 tested official stock MOS3.0.2 and
R0046 the previous full OTIRX implementation; both passed. R0042..R0044 remain
unchanged. Their installed-ROM checks match every selected image. All30timed
trials passed,245760receiver payload bytes were verified, and no paced
completion deadline miss was recorded. next.txt is47 and every journal START
has the expected PASS.

The important result is that official MOS and unmodified Eirene perform almost
identically in this fixture. The earlier bulk-speed result is therefore not
explained by choosing Eirene as the new baseline. None of the asynchronous
variants beat official stock's bulk rate here. The strong benefit is application
calculation headroom, and the small Eirene OTIRX implementation delivers more
of that headroom than the previous full implementation.

## Hardware results

Medians of three samples per workload per case. kB/s below means1000payload
bytes/second, excluding command headers; percentages use official stock MOS.
These are completed calculation rates, not direct measurements of CPU usage.

| Firmware / sending path | Bulk payload kB/s | Bulk change | Paced useful-work gain |
|---|---:|---:|---:|
|Official stock MOS3.0.2|107.40|baseline|baseline|
|Unmodified Eirene|107.37|-0.03%|-0.03%|
|Minimal Eirene scalar async|105.35|-1.91%|+59.52%|
|Minimal Eirene OTIRX async|106.06|-1.25%|+64.51%|
|Previous full OTIRX borrowed/direct|98.93|-7.89%|+51.15%|

Against the previous full implementation, minimal OTIRX completed8.84% more
foreground calculation and achieved7.21% higher bulk rate in this fixture.
It still did not exceed stock bulk rate. The ordinary-output results differ
by only0.03%, far below a useful distinction in this small sample set.

The frozen waveform/workload is8192payload bytes in32public buffered-VDP
commands (8448wire bytes), with the same256-byte pacing at44100payload B/s,
PRT timing, comparator and calculation kernel across cases. All calibrations
completed229work units. The two added apps use separate untimed identity/selection
checks; the old full implementation necessarily has its UTX1 API adapter and
session/configuration costs. These are comparisons of complete callable paths,
not a claim that a single instruction accounts for their whole difference.

Three short bulk samples do not establish indefinite sustained rates or
statistical precision. The first three cases and two added cases were run in
separate batches; no claim of randomized/interleaved five-way testing follows.
The existing protocol checks every payload byte and validates work checksums,
callback counts and the two PRT counters. This is not a44100single-byte IRQ/s
audio-latency qualification, real-app trial or electrical CTS-margin measurement.

## Earlier throughput claim

The historical comparison recorded98raw ticks for its blocking reference and58
for both experimental per-byte async and OTIRX. That remains evidence about
that earlier fixture and its modified-MOS blocking path. It is not evidence of
a69% advantage over separately flashed official MOS under this matched fixture.
The five-case result does not reproduce that advantage; do not explain the
old result as a stock-Eirene improvement. Identifying the original discrepancy
would require an audit of the old reference path/workload/timing before quoting
it as a general transport speedup. No such cause is asserted here.

## Integrity, state and custody

Returned apps, firmware, comparator, case registry and other immutable deployed
files match the deployment receipt. R0042..R0044 remain byte-for-byte unchanged;
only the two new reports and the normal journal/counter updates appeared.
All five build/case identities resolved uniquely. Both new installed-MOS prefix
CRCs match: official15BD0583, previous full OTIRX6E17923C.

The last test establishes the previous full experimental MOS was installed then;
no later restoration was reported. The matched receiver-fixed VDP is staged and
was required by the protocol, but the reports do not independently fingerprint
installed VDP firmware. Leaving the dedicated Console8 experimental between
UART tests remains authorized; restoration was not a missing test step.

The card was inspected read-only. Raw complete appliance, all five reports,
combined comparison and detailed validation are on Pi and Oryx at
`build/tm001/hardware-five-r01`. Pi archive:
`build/tm001/hardware-five-r01.tar.gz`, SHA256
2d7828333f0e1d236e1731dca59aa08fe79dd80bb3eed32dd19348282c490bf5.
The JSON companion contains every parsed sample, returned-file hash and
case comparison. Ignore unreliable Agon file dates; use report numbering.

The controlled five-case comparison is complete. This favors the small Eirene
OTIRX candidate for the next real-application demonstration. It does not approve
a release, EMOS fit, or compatibility of unmodified applications with the new
opt-in syscall. The planned next application step remains adapting the balls
demo to the small Eirene single-buffer interface.
