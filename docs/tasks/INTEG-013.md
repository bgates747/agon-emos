# INTEG-013 — Owned mainboard SD-service transport gateway

Author released Extender PORT-017 on 2026-09-12; this is its EMOS component.
Highest EMOS priority until the SD capability is accepted. Wire contract:
agon-extender `docs/tasks/PORT-017/PROTOCOL.md`; this task owns maintained eZ80
transport code and qualification, not the P4 or application filesystem logic.

1. Add reserved `ext.sdlink` to the existing public gateway, preserving all
   existing APIs and ordinary VDU/input behavior. Validate request/output memory,
   Legacy mode, Extender keyboard ownership, operations and lengths.
2. Receive bounded private 8D service records through the existing UART1 parser.
   Keep one mailbox and never invoke filesystem work or game keyboard callbacks
   for these packets. No direct application UART/vector/register access.
3. Send private F6 envelopes only through the EMOS-owned writer. Preserve
   keyboard receive interrupts. Invalidate admission/mailbox on fault, application
   entry/exit and explicit close; no dangling application pointers in the ISR.
4. Prove the new gateway fits the 128KiB ROM without dropping existing behavior.
   Build using wrapper firmware-check and every profile link guard. Baseline
   reproduced at 130344 bytes; only 728 bytes are currently spare. Keep bulk
   filesystem and CRC logic in the foreground application.
5. Test malformed/bounded/lifecycle/concurrency behavior on host and headless
   target. Freeze exact candidate inputs, preserve recovery, and establish
   firmware deployment authorization before flashing EMOS. Coordinate physical
   qualification through Extender PORT-017. Do not begin graphics/Golem work.

Current phase: implementation gate. No firmware has been changed. Source edits
are provisional until ROM/link and behavioral qualification. Existing private
QTG/8C diagnostic behavior remains intact; no repurposing it for file bytes.
