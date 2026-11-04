# RC1 final Console8 check — 2026-11-03

The final check passed. R0052 and R0053 both match fixture77e90fa738100d1d and
installed MOS CRC323F792F8D over101140bytes, the exact branded RC1 image. Each
run checked8192payload bytes, totaling16384, with no missing or failed report.
Both have their matching journal START/PASS; next.txt is54.

Both reports passed ordinary RST18 before opt-in and after finish, comparator
negative/positive controls, zero-length/masked-context refusal, boundary
lengths1/15/16/17/255/256/257/1024, NULL callback, second-write BUSY, source reuse,
concurrent RX and the receiver's complete payload comparison. An input-only
keyboard hook was installed; actual keyboard events were not injected.

The Author changed the final protocol to stock Bistromathics VDP2.16.0 and then
reported all tests appeared to pass. This superseded the earlier staged
readme's matched diagnostic-VDP instruction. Stock VDP identity, cold-boot banner
and C-example success are operator-protocol/completion evidence; the fixture
does not fingerprint VDP or log those visual observations independently.
The exact MOS identity and repeated receiver checks are recorded by the reports.
No additional confirmation or bench run is required by the agreed final gate.

The sender introduces no VDP protocol change. The new result qualifies this
focused normal-workload regression on stock VDP; it is not another performance
benchmark or a retest of the separately diagnosed receive-recovery race.
Earlier64.06% paced useful-work improvement and85.5% no-traffic calculation-rate
retention during bulk sending came from the pre-branding build with the
receiver-fixed VDP. Do not relabel those timings as stock-VDP measurements.

## Exact artifacts and custody

Frozen source:47dd56129c0043931bb32f7d329c0392a0b8a809.
MOS SHA256:49873fb817f47cc76f50b8cec1c3d9cde7b252ac2ec844e08e745bdaf1c5eeab.
The MOS, test/example applications, comparator, maps and other immutable card
files match deployment. Only R0052/R0053 and ordinary journal/counter updates
appeared. Reports use sequential numbers; unreliable Agon timestamps were
ignored. The card was inspected read-only and was not flashed or modified.

The complete returned appliance was archived and verified on Pi and Oryx at
build/tm001/rc1-hardware001/returned.tar.gz, SHA256
6cb81fabe030447d8aa2a643651658cd43fa4ea0c36fb0f98f2ca52af87b648a.
Extracted files, custody receipt and analysis.json are beside it.
[Raw reports/journal](hardware/) and [full identities/custody](hardware.json).
The cold-boot/example observations are not promoted into machine-log claims.
Branded-image emulation was waived by the Author and was not run.

The final ZIP adds these results and correct stock-VDP instructions, removing
the draft's pending status. Its firmware and every frozen MOS/generated-app
source and binary remain byte-for-byte unchanged from the qualified package.
Final ZIP and checksum are at build/tm001/rc1-final001/ on Pi and Oryx.
RC-003 is complete. RC-005 publication is now authorized by its existing task.

[RC-005 publication is now complete](publication.md).
