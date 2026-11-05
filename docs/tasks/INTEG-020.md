# INTEG-020 — Foreground parallel network admission

Move opaque application network records through an explicitly selected parallel
carrier while preserving Legacy mainboard graphics and normal input. Extender
NET-004 owns the cross-component contract, accepted with MechAgon under Smith's
direct ABI delegation on2026-11-05. No installation or emulator changes selected.

## Scope and work

I20-001 Publish this owner plan before source edits. [x]

I20-002 Add an optional ext.netp gateway with full caller-span validation,
  no retained app pointer/callback, boot-local nonwrapping session, one pending
  command and explicit busy/empty/fault/undersized results. [ ]

I20-003 Bind through EMOS-owned Legacy service parallel admission, preserving
  physical parking/return and unrelated UART0 ISR/VDU routing. An absent physical
  binding must refuse, never use UART payload. [ ]

I20-004 Exercise actual C endpoint with finite host adapters, then native wrapper
  build and all inherited linked checks; measure ROM/BSS. No full fit claim from
  host objects. [ ]

I20-005 Match owner P4 candidate and separately qualify hardware under normal
  Extender checkout. Keep normal startup and owner-retained rollback. [ ]

## Research and boundaries

Exact installed baseline5735812; prior ext.net uses UART1 and borrowed callback.
Official MOS0x51 is the existing project gateway, not a new global allocation.
Cross-component experimental contract defines ext.netp OPEN/SUBMIT/TAKE/RELEASE,
NM14..142B and128B application payload. Application owns interpretation,
correlation/deadlines and foreground polling. Resident EMOS validates buffers,
committed Legacy/onboard route, admission and bounded physical lifetime only.
No queue, checksum, game logic, renderer or automatic replay. IRQ-off and UART0
CTS coexistence require measurement. Old ExExt-only admission is not permission
for a Legacy service. Pending physical bindings are explicit, not claimed ready.


## Source checkpoint

The optional gateway and NP metadata binding are implemented; native C SDK223B
application state and real-gateway sanitizer checks pass. Physical entry/return
reuses the SD-upload sequencer;108 paired donor coordinator cases and15 upload
pair cases pass with modeled byte I/O. The exact physical loop and register guards
remain, with no new callback or checksum. One token replaces duplicate resident
copies, including ownership retained on a post-admission fault. The uninstalled
raw native image is131002B/70free; full profile guards, identified image and RFC25
splice remain pending. Stored initialized-RAM bytes are now included in the
linker’s physical-ROM bound. No target operation or firmware adoption occurred.
