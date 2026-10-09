# UART flow-control diagnostic MOSlet

Development candidate: UARTFLOW's existing sequence and reporting now live in
an SD utility. EMOS retains only admitted UART access and lifecycle cleanup.
Host and linked eZ80 checks pass. A bounded P4-PC hardware run also passed
paired success, wrong-peer failure and ownership cleanup; Author review and
production promotion remain separate.

Build with `make` in this directory and the normal `AGONDEV_TOOLCHAIN` setting.
Install only `bin/uartflow.bin` as `/emos/uartflow.bin` on a qualified development
card. Invoke `EMOS UARTFLOW`; `EMOS UARTFLOW --version` identifies the utility.
The utility occupies the stock `0xB0000..0xB7FFF` MOSlet slot. It returns zero
on pass, 15 on diagnostic failure, and 19 for invalid arguments.

EMOS must contain the development `ext.uartdiag` v1 service. Older firmware
rejects the request; a missing utility produces the ordinary load error.
This does not change the selected production firmware or install procedure.

The test requires Legacy mode, mainboard keyboard selection and no other UART1
owner. If that physical keyboard is unavailable, use a prepared noninteractive
startup with an independently verified recovery path. The P4 must run the dedicated FLOW/FLOWACK flow-control test peer; the
ordinary Extender console is not that peer. Missing/wrong peers cause a bounded
failure. Establish input readiness before changing keyboard selection: remote
Extender input cannot operate while this test owns UART1. The P4's separate
capture must attest its blocked-return timeout; the MOSlet alone cannot prove it.

The utility preserves the previous MOS-clock deadlines and stalled-clock guard.
EMOS performs bounded individual RX/TX/RTS operations at 1,152,000 baud, 8N1,
with hardware flow control. The utility owns no GPIO registers, IRQ vectors or
resident callbacks. Explicit close and EMOS utility-exit cleanup release the UART.
The API-call overhead changes polling cadence; physical timing must not be inferred from the host or CPU-model tests. The
2026-10-08 paired hardware run passed its endpoint pause/resume/timeout checks;
no new logic-analyzer baud/edge measurement was performed.

See [resident service contract](../../docs/emos-v1-contract.md),
[utility dispatch](../../docs/emos-utilities.md), and
[implementation task](../../docs/tasks/INTEG-016.md).

UARTTEST/VDPPOLL now share this gateway adapter in the newer off-bench
composition. The additive slow-open operation does not change UARTFLOW's
fast-lease algorithm or qualification history; see
[probe MOSlets](../uartprobe/README.md). Their hardware validation is separate.
