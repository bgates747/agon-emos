# Small C/AgonDev UART0 example

The built RC package contains example.bin and the complete generated source.
With the matching RC MOS installed, type `example` from its directory. The
example checks the ROM CRC before using the fork-local42h/43h syscalls, submits
one static text source, calculates while the interrupt handler sends it, then
finishes before printing RC1 EXAMPLE PASS and its work count/checksum.

The callback only posts a flag. Arm it before submission; keep source and
callback storage live. Do not interleave printf/RST18 while the sender owns
UART0. The tiny global-argument bridge is single-foreground-caller code, not a
reentrant/ISR-safe library. See ../../docs/uart0-async.md for the full contract.

The release preparation script generates identity.h from the exact MOS image
and copies the shared ROM checker into the frozen example source. Then build
with `make AGONDEV_TOOLCHAIN=/absolute/path/to/agondev`. For the repository tree,
use the reproducible recipe in ../../experiments/uart0-release/README.md.
