# MI-005 result: controlled comparison ready for Console8

We now have one test program that measures how much calculation an application
can do while sending a regularly paced stream, and how fast it can send an
unpaced block. It compares ordinary Eirene output with the small Tom-derived
scalar sender and its OTIRX version. All three passed the same receiver checks
in Flexe. The hardware comparison is ready; these emulator runs do not decide
which sender is fastest on a real Agon.

The sender firmware is unchanged from the preceding tranche. This work adds
an original C/AgonDev fixture, timer/ABI bridges, result analysis and a manual
appliance. No extra MOS recovery or robustness framework was added.

## Pins

| Item | Identity |
|---|---|
| Fixture source | b4bde8d3a76e8fb0686bd656feaa642966ce7ec1 |
| Analysis tools | a83d538781f9f4f57d6cd09340f24fcc20ecf898 |
| Fixture build | 1ad95f651c6e017d |
| test.bin | 24920 bytes |
| test.bin SHA256 | 0f817a364acd229d48fac395f81aec486a7f64963523d6ee4b4d01cc14583490 |
| test.map SHA256 | 462b14e3825f7ce06c7a0b35de3708aa6abb17df3290f8db8747d895cf391cd6 |

MOS images, maps, source commits and maintained frontend/backend/VDP pins are
unchanged from [functional-r02.md](functional-r02.md). Each firmware selection
is checked against its exact installed ROM prefix before any experimental
syscall: pristine 100660 bytes/CRC32 B4869566; scalar 101163/112ACBBF;
OTIRX 101155/B201C2A4. The prefix excludes unused flash tail. The app source
snapshot matches the committed functional sources byte for byte.

## Final checks

| Sender | JIT | Interpreter | Verified payload per cell |
|---|---|---|---:|
| Pristine Eirene ordinary output | PASS | PASS | 49152 bytes |
| Lightly fixed Tom-derived scalar | PASS | PASS | 49152 bytes |
| Same sender with OTIRX | PASS | PASS | 49152 bytes |

Each cell ran three bulk and three paced trials. In total, 36 timed trials and
294912 receiver payload bytes passed. Both comparator controls passed before
use; independent analysis also checked all six complete 8448-byte outgoing
streams and every receiver-comparison command per cell. Work checksums,
callback counts, installed-ROM pins and the two timer counters agreed. No
paced completion deadline was missed in this model cohort. Both async paths
returned to the ordinary CLI. There were no UART drops/RX overruns or backend
failed/unhandled/unregistered/unmapped operations.

The host analyzer rejected deliberately altered callback counts, work checksums,
clock fields and receiver-success markers. The returned-report comparison tool
was exercised on matched model reports, explicitly labeled as emulator data.
It calculates median bulk rates and useful-work rates, their percentage changes
from ordinary output, and missed deadlines. These are not direct CPU-cycle
measurements; work counts measure available foreground capacity.

The detailed measurement boundaries and limits are in [performance.md](performance.md).
Paced traffic is 256-payload-byte chunks averaging44100 payload B/s, not a 44100Hz
single-byte scheduling or audio-underrun test. Local drain excludes the separate
remote execution fence. Logging, comparator commands and SD reads are untimed.
Bulk samples are 8KiB, not unlimited sustained-load qualification. No EMOS,
physical CTS reserve, real-app or hardware-performance claim is made.

## Prepared manual appliance and custody

Oryx directory:
`/home/smith/Agon/mystuff/eirene-mos/build/tm001/console8-perf002`.
Pi mirror: `/Agon/mystuff/eirene-mos/build/tm001/console8-perf002`.
The short [readme.txt](/Agon/mystuff/eirene-mos/build/tm001/console8-perf002/readme.txt)
contains the three explicit firmware selections/runs. All image/file hashes
passed SHA256SUMS. The package contains the same qualified test.bin/checks.bin,
pristine/scalar/OTIRX MOS, matched receiver-fixed VDP, standard MOS/VDP restoration
files, source/build pins and retained licenses/credits. **It has not been copied
to an SD card. Physical flashing/execution remain manual Author actions.**

Final raw profiles/reports/wire captures are at `build/tm001/perf-flexe002` on
both machines; the frozen build is `build/tm001/perf003`. The JSON companion
contains parsed samples, pins and release evidence. Common runtime binaries and
full private card images remain on Oryx, with their hashes recorded locally.

Preliminary perf-flexe001 also passed six cells. Review then removed a printf
from an unexercised timeout path because it would compete with a still-live
accepted transfer; the final exact binary was rebuilt and all six cells rerun.
This was a fixture fix, not a sender-firmware change. Original attempts remain
in workspace history. Initial staging/build/package attempts missing a shared
generator or license stopped before any hardware or card write; final inputs
were supplied and fresh snapshot names used.

Both shared Oryx leases 203 and205 were released with fresh empty-host evidence.
No next request was waiting at either release. The second reservation waited
normally behind Golem's checkout 204; no lease remains held by this project.

MI-005 and PF-001 through PF-007 are complete. TM-005 remains open: deploy the
prepared appliance when the Console8 card is available, have the Author run the
three firmware selections, archive returned reports and assess physical timing
before adapting the visible balls demo or claiming application gains.

Verified Pi archive: `build/tm001/performance-r02.tar.gz`, SHA256
`08c44e64356b7a56496b72d217339204a277a47220c415d51c52cbf89496cb7e`.
It contains the exact application build, all three MOS binaries/maps, prepared
appliance, both cohorts' raw evidence, and complete pinned pristine/experimental
source trees (with the scalar one-line delta). Runtime binaries/card images
remain separately pinned on Oryx. A copy of this archive is also retained there.
