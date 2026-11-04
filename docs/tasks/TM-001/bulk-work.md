# Five no-argument fixtures and bulk calculation headroom

The Author requested a separate no-argument fixture for each of the five MOS
images, covering both existing scenarios and the previously unmeasured useful
foreground work during maximum-rate bulk sending. Physical Console8 flashing
remains manual. Firmware images are reused unchanged.

BW-001 [x] Implement one common PERF2 measurement core with three repeats each
of bulk-only, paced-plus-work and bulk-plus-work. Keep local source release,
local UART drain and remote execution distinct. Continue useful foreground
calculation through nonblocking drain checks in bulk-plus-work. Count/checksum
one xorshift step per work unit for finer observation resolution; report raw
work and a conservative credited count discarding the potentially straddling
last unit in bulk-plus-work. Report throughput change against bulk-only as
well as useful work rate and fraction of the no-traffic calibration capacity.
This measures foreground work, not unused hardware cycles or arbitrary work
inside application ISRs. PERF1 and PERF2 units are different and must not be
merged as one performance cohort.

BW-002 [x] Build stock.bin, eirene.bin, scalar.bin, otirx.bin and ours.bin from
the identical core and external configuration data. Each knows its MOS image,
checks its exact ROM prefix and runs its full suite when invoked by name with
no switches. Avoid compile-time specialization of measured paths. Use the
appropriate minimal Eirene or previous UTX1 adapter; borrowed/direct callback,
batch16 and copied RST18 OFF for the previous full implementation. Common
payloads, source boundaries, clocks, callback bodies and receiver checks apply.

BW-003 [x] Qualify all five fixtures through actual plain MOS command invocation
on guarded Flexe profiles for both engines. Preserve results and release the
shared Oryx checkout normally. Validate trial completeness, checksums, callback
counts, payload streams, timers and report identities. Functional PASS does
not mean a performance target was met. No emulator speedup claims.

BW-004 [x] Deploy the five fixtures and firmware in /test/mos-uart0 on Console8's
identified card. Keep the prior complete package in previous/ and preserve its
reports/versions; continue sequential report numbering. Supply concise readme
instructions using only flash commands and each fixture's plain name. Gather
all three scenarios in one report per fixture. The Author runs all five new
fixtures; previous physical results remain historical, not a substitute for
the newly added bulk-plus-work measurements.

BW-005 [x] Author runs all five new fixtures on Console8. Archive returned
reports and analyze all three scenarios across the five images. Do not reuse
PERF1 work counts for the new cohort.

Implementation, ten functional cells and card deployment are complete; see
[the ready record](bulk-work-ready.md). Physical PERF2 results remain pending.

All five physical suites and returned-report analysis are complete. See
[Console8 results](hardware-perf2-r01.md):45trials passed, no missing cases.
