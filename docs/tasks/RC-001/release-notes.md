Applications can hand one immutable buffer to MOS and keep calculating while
UART interrupts send it to the VDP in 16-byte OTIRX batches. Ordinary RST18 and
startup retain their Eirene behavior; applications must explicitly opt in.

This is **Eirene MOS 3.1.0 UART0 RC1 in bgates747's fork**, based on Tom Morton's
`3.1.0_beta_1` and supplied small async sender. It is not an upstream Eirene
release. The candidate is on `codex/uart0-async`; `main` is unchanged.

The exact RC image passed two consecutive Console8 regression runs using stock
Bistromathics VDP 2.16.0 under the agreed operator protocol. R0052/R0053 match
MOS CRC32 `3F792F8D` and verify all 16,384 payload bytes, ordinary output before
and after async use, callback/NULL callback, boundaries, source reuse and
concurrent RX. Cold boot and the C example passed per the operator's completion
report. Raw reports and qualification scope are included in the ZIP. No new
VDP protocol or modified VDP is required for this final regression.

The fork-local interface uses syscalls `42h`/`43h`, one borrowed source, an
optional bounded completion callback and explicit nonblocking finish. The ZIP
contains exact MOS source/binary/map, an original C/AgonDev example, regression
fixture source/binary, comparator, checksums, installation/restoration
instructions, original licenses/credits and evidence. Use the ordinary
`flash mos mos-rc1.bin` command, then run `test` and `example` by name.

Earlier controlled Console8 tests of the sender before RC branding found
64.06% more foreground calculation during paced sending than official stock
MOS. During flat-out sending it retained 85.5% of its no-traffic calculation
rate at essentially stock throughput. Those benchmarks used a separately
receiver-fixed VDP; they are not new stock-VDP performance measurements or
exclusive CPU-utilization figures. Other boards, EMOS and real application/audio
deadlines remain separate qualification work.

Source commit: [47dd561](https://github.com/bgates747/eirene-mos/commit/47dd56129c0043931bb32f7d329c0392a0b8a809).
Read the included caller contract: finish before ordinary output or returning.
All original copyright/license notices are retained, including Dean Belfield,
Tom Morton and the upstream Agon contributors.
