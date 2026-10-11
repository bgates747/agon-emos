# Private two-way parallel qualification MOSlet

Revision r02 follows CRC-free parallel wire3 (CRC-REMOVAL-R01); r01
remains historical wire2 evidence. Descriptor padding is ignored; the payload
CRC below is independent qualification evidence, not transport admission.

PORT-008 LC03-A only; requires the exact paired probe firmware. Ordinary
EMOS refuses operation6. No public API or formal ExExt support is added.
The MOSlet asks EMOS to negotiate/admit one bounded block; EMOS owns all pins,
UART state, native assembly, completion and fault fencing.

Build with the maintained AgonDev toolchain, then deliberately install as
`/emos/parprobe.bin`. Invocation (only after a separately approved deployment):
`EMOS parprobe /agents/extender/results/UNIQUE.csv`.
Prepare that directory beforehand. The MOSlet refuses an existing result and
stops at the first failed case. It never switches video mode. It runs twenty
blocks:1,2,255,256,4096 bytes in both directions, twice. Reverse patterns depend
on the negotiated descriptor; forward patterns change each round. Guards and
every byte are checked on the Agon. Forward integrity additionally requires
matching its CRC/first/last values with the P4 USB record; MOSlet success alone
is not a forward endpoint pass.

Operation6 uses ordinary API51h ext.uartdiag MOSlet/Legacy/busy checks. Input
is two bytes `[6,direction]`; output capacity is16+payload length, at most4112,
wholly in the MOSlet slot. On success EMOS returns the exact16-byte offer before
the payload. On failure all output is provisional. EMOS retains session identity
and generates a new transaction before every block. No direct resident-address
binding, raw GPIO, VDU route or interrupt-vector manipulation occurs.

CSV records start/end and per-service-call MOS ticks (nominal120Hz, not measured
PRT time). P4 logs microseconds from offer admission to receipt consumption;
this includes handshake/driver/control overhead and is not link-only throughput.
No physical pass or install readiness follows from building the MOSlet.
