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

1. [ ] Publish/freeze plan and exact baseline before source edits.
2. [ ] Agree the concrete gateway layout with Extender's application include
   before parallel edits; document fixed endpoint/admission and old-ABI refusal.
3. [ ] Remove resident semantic validation, source/edge tracking, request body
   policy and neutralization. Retain only required bounded correlation, copy,
   age metadata and expiry. Measure smaller alternatives before keeping them.
4. [ ] Update the actual resident harness and linked controller regression for
   new operations, invalid buffers/callers, pending/busy state, failed sends,
   clock wrap, delayed replies, expiration and keyboard/Escape interleaving.
5. [ ] Build full ordinary/controller images through maintained wrappers and
   mandatory link checks. Compare identical identities/configuration against
   the frozen DIAG-003 baseline; report complete ROM/RAM change and object cost.
6. [ ] Preserve source/artifacts/evidence, update current contracts/logs, and
   report the offline result with physical/version/promotion gates remaining.

## Research boundary

Official MOS API/Executable contracts remain read-only in the workspace's
agon-docs. Existing ext.controller already shares EMOS gateway, keyboard UART
parser/serializer, clock and lifecycle. Its semantic interpretation duplicates
application work. Stock VDU/sysvar queries do not supply an equivalent generic
96-byte UART1 reply. SD transport has different memory admission/ownership and
an interrupt-masked read pattern unsuitable for controller reuse. Extender's
INPUT-005 detail/audit pins the relevant source and current wire contract.
