# INTEG-018 — Minimal resident controller endpoint

The Author directs maximum practical recovery of EMOS ROM by moving controller
logic into a reusable application include (their personal API), not a MOSlet.
Extender INPUT-005 owns that include, viewer integration and cross-component
contract. This component owns the smallest correct resident delivery endpoint.

## Authority, baseline and limits

1. Authorized 2026-10-30. Publish/commit/freeze plan and current dirt before
   implementation. Component source starts clean at eebdd76 after diagnostic
   extraction. Its controller object is 2,199 ROM bytes; full unversioned image
   127,817 / 3,255 free. Reuse those exact retained comparison artifacts.
2. Work offline: source, complete wrapper builds and native/linked checks. No
   hardware, firmware installation, SD/startup, emulator or production change.
3. Preserve EMOS UART/mode ownership, ordinary MOS compatibility, keyboard/SD
   traffic and existing P4 CI2/CIS2 bytes. No checksum, raw application I/O or
   SD-lease borrowing. Do not implement a configurable generic transport.

## Decisions

| ID | State | Decision |
|---|---|---|
| I18-01 | Author accepted | Put every practical controller-specific duty into the application include; keep only justified resident transport/delivery work. |
| I18-02 | Selected implementation | Reuse gateway, serializer, UART parser, clock and lifecycle. EMOS retains one bounded coherent reply and essential caller/correlation/deadline guards. |
| I18-03 | Selected implementation | Use distinct new operations, copied raw wire state plus timing metadata and an application-built fixed controller request. Move content validation, source/ACK bookkeeping, request sequence and age interpretation to the application. Old services/clients fail clearly; do not retain two semantic implementations. |
| I18-04 | Constraint | No MOSlet or transient pointers in ISR. Copy bulk data with interrupts enabled and revalidate lifecycle before consumption. |

## Work and validation

1. [x] Publish/freeze plan and exact baseline before source edits.
2. [x] Agree the concrete gateway layout with Extender's application include
   before parallel edits; document fixed endpoint/admission and old-ABI refusal.
3. [x] Remove resident semantic validation, source/edge tracking, request body
   policy and neutralization. Retain only required bounded correlation, copy,
   age metadata and expiry. Measure smaller alternatives before keeping them.
4. [x] Update the actual resident harness and linked controller regression for
   new operations, invalid buffers/callers, pending/busy state, failed sends,
   clock wrap, delayed replies, expiration and keyboard/Escape interleaving.
5. [x] Build full ordinary/controller images through maintained wrappers and
   mandatory link checks. Compare identical identities/configuration against
   the frozen DIAG-003 baseline; report complete ROM/RAM change and object cost.
6. [x] Preserve source/artifacts/evidence, update current contracts/logs, and
   report the offline result with physical/version/promotion gates remaining.

## Research boundary

Official MOS API/Executable contracts remain read-only in the workspace's
agon-docs. Existing ext.controller already shares EMOS gateway, keyboard UART
parser/serializer, clock and lifecycle. Its semantic interpretation duplicates
application work. Stock VDU/sysvar queries do not supply an equivalent generic
96-byte UART1 reply. SD transport has different memory admission/ownership and
an interrupt-masked read pattern unsuitable for controller reuse. Extender's
INPUT-005 detail/audit pins the relevant source and current wire contract.

## Offline result — 2026-10-30

Plan/freeze commits were Extender `b9c844aa` and EMOS `379bf34`. The resident
controller object falls from 2,199 to 1,208 ROM bytes (45.07% smaller), and 131 to 102
static RAM bytes. Complete controller EMOS falls 127,817 to 126,826 bytes, leaving
4,246 of 131,072 bytes free; static RAM falls 4,189 to 4,160. Ordinary EMOS remains
byte-identical at 125,520 ROM /5,552 free. Equal unversioned identities exclude
string-length differences from these savings. Initial extraction saved 988 bytes;
selective lock/copy placement measured and recovered another 3 without removing
any guard. Both complete profiles pass all wrapper-owned linked checks.

All 145 EMOS host tests and 43 Extender controller tests pass. The final linked
controller model watches all 96 output stores with interrupts enabled and delivers
Escape through the actual IRQ; correlation, one-shot reply, expiry/wrap, late
reply, old-operation refusal, caller/buffer/mode and failure cleanup pass.
Another 298 diagnostic and 21 UART-reservation cases pass on the same linked image.
No physical timing is inferred from instruction counts.

The application include and mode20 viewer build on AgonDev. Extender's
[INPUT-005 results](../../../agon-extender/docs/tasks/INPUT-005/RESULTS.md) retain
portable metrics, exact artifact hashes, application cost and compatibility
limits. Existing P4 CI2/CIS2 bytes are unchanged; new EMOS operations 4–7 refuse
old 0–3 clients. Deployment/version selection, physical review and production
acceptance remain pending. No hardware, SD/startup, emulator profile/snapshot or
production selection was changed.
