Applications can hand one immutable buffer to MOS and keep calculating while
UART interrupts send it to the VDP in16-byte OTIRX batches. Ordinary RST18 and
startup retain their Eirene behavior; applications must explicitly opt in.

This is **Eirene MOS3.1.0 UART0 RC1 in bgates747's fork**, based on Tom Morton's
3.1.0_beta_1 and supplied small async sender. It is not an upstream Eirene
release. The candidate stays on codex/uart0-async; main is unchanged.

The register interface uses fork-local syscalls42h/43h, one borrowed source,
an optional bounded completion callback and explicit nonblocking finish.
The ZIP includes exact MOS source/binary/map, a small C/AgonDev example,
no-argument regression fixture source/binary, comparator, checksums,
installation/restoration instructions, licenses and qualification records.

Controlled Console8 tests of the sender implementation before RC branding
found64.06% more foreground calculation during paced sending than official
stock MOS. During flat-out sending it retained85.5% of its no-traffic calculation
rate at essentially stock throughput. These are calculation measurements,
not exclusive CPU-utilization figures or a demonstrated bulk-speed improvement.

Use the normal `flash mos mos-rc1.bin` command. Read the included README and
caller contract before using the new syscalls; finish before ordinary output
or returning. The sender adds no VDP protocol extension. Receiver qualification
used the separately fixed VDP2.16.0 described in the evidence. EMOS, other boards
and real audio deadlines remain separate qualification work.

Source:47dd56129c0043931bb32f7d329c0392a0b8a809.
All original upstream copyright and license notices are preserved; credits
include Dean Belfield, Tom Morton and the upstream Agon contributors.
