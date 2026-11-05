# INTEG-021 — RFC25 UART0 in the matched parallel network composition

Combine the Author-selected Tom Morton UART0 sender with the new parallel
network gateway so MechAgon can render through mainboard Legacy UART0 and move
network records through the parallel pipe. This is a maintained EMOS splice,
not installation of the standalone Eirene ROM. No emulator or hardware operation
is selected by this source plan; keep the installed development pair intact.

## Authority, references and contract

Smith selected RFC25 for MechAgon UART0 and delegated the cooperating owners’ ABI
and implementation. UART owner supplied the reviewed minimal-splice advice in
its RFC-001 consumer-coordination/emos-splice-advice.md: Tom sourceffab9c9,
11B BSS/FIFO16 OTIRX/WAIT_CTS, TX classification before the existing RX saves,
callback source release distinct from TEMT/receiver DONE. Preserve corrected
native startup and every existing RX/keyboard/VDPlet callback save/restore.
Official docs mos/API.md describe existing output/callback contracts and do not
allocate slot25; official docs f9806bd/MOS8336409/VDPc7ac293 are read-only.

I21-001 Publish owner plan/TODO before source implementation. [ ]

I21-002 Implement optional matched RFC25 profile, public slot25, ADL/MBASE0/EI,
  immutable positive full24-bit span and primary-register flag callback. Name
  explicit admission results0/19/31/32/35; no callback on refused submission.
  Keep actual Legacy/backend0 and physical owner exclusion atomic. [ ]

I21-003 Preserve Tom’s TX ISR stack and FIFO/CTS code; retain ownership until
  source release AND actual TEMT. Consumer additionally waits tagged module DONE
  before a netp epoch. No resident finish/queue/cancel or background game logic.
  Keep old experimental42/43 only in their original profile; never run both TX
  engines concurrently. Mainboard ordinary output routing remains unchanged. [ ]

I21-004 Exercise linked native TX/admission/ISR and callback register behavior,
  rejection without mutation, full24-bit count, final FIFO and tail ownership.
  Measure ROM/BSS and keep all five inherited composition guards enabled. [ ]

I21-005 Publish exact matched build/header/map to MechAgon after source checks.
  Keep hardware deployment/finite qualification, normal checkout, owner-retained
  rollback and acceptance separate; no unperformed concurrency or speed claim. [ ]

## Proposed narrow owner decisions

1. New matched profile uses25; historical experimental42/43 are unavailable in
   this profile, while the original default profile retains them. Ordinary MOS
   interfaces remain backward compatible. No hidden RC2 finish substitution.
2. Callback is an ADL assembly leaf called in TX IRQ with primary AF/BC/DE/HL
   saved by the ISR; preserve IX/IY/alternate registers if used. No MOS, wait,
   allocation or reentrant submission. A flag-store/RET leaf is sufficient.
3. Data/callback addresses are ordinary application RAM40000..AFFFF; length is
   full24-bit positive and checked against its remaining span. Rejection keeps
   caller registers and entry interrupt state. Busy admits no source bytes.
4. A netp OPEN logical session can coexist with UART0 graphics. Only active
   physical epochs exclude UART0 admission; netp physical entry refuses through
   source release/TEMT. Do not exclude UART0 for the whole network session.
