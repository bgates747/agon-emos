# RC2: publish the smaller UART0 sender

RC2-001 [x] Preserve published RC1 as the hardware-verified guarded alternative.
RC2-002 [ ] Build RC2 branding reproducibly; prove only version.h/main.o differ
from hardware-tested TRIM. Do not claim exact branded-image hardware testing.
RC2-003 [ ] Package matching MOS and worked-example/test sources and binaries,
caller contract, how-tos, evidence and checksums in a downloadable ZIP.
RC2-004 [ ] Publish a fork prerelease on the project branch; preserve main/RC1.

Author authorized publication on 2026-11-04. RC2 changes caller obligations:
foreground ADL1/MBASE0/EI, sole normal-UART ownership, and app-side preservation
of callback IX/IY/alternate registers. The C example supplies a fixed-target gate.
Same performance, smaller sender; RC1 remains a valid more guarded alternative.
