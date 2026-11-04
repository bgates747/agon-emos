# Small C/AgonDev UART0 example

This working-tree example targets the unpublished TRIM trial. The RC1 tag and
published package retain their original example and stronger callback contract.
With the matching trial MOS installed, type `example` from its directory.
It checks the ROM CRC, submits one source, calculates while MOS sends it, then
finishes before printing TRIM1 EXAMPLE PASS and a work counter/checksum.

The C callback only posts a flag. Its fixed-target assembly gate preserves
IX/IY and alternate registers outside MOS. A different C callback needs its own
gate; never replace an active callback through a mutable shared dispatch slot.
Arm completion state before submission and keep source/callback storage live.
Finish before printf/RST18 or returning. The argument bridge is private to a
single foreground caller, not reentrant or ISR-safe.

[Trial contract](../../experiments/uart0-trim/contract.md) and
[build recipe](../../experiments/uart0-trim/README.md). The preparation script
generates identity.h and copies the ROM checker from the matching firmware.
Build the generated source with AgonDev using the pinned compiler/libraries.
