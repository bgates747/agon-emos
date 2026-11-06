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

I21-001 Publish owner plan/TODO before source implementation. [x]

I21-002 Implement optional matched RFC25 profile, public slot25, ADL/MBASE0/EI,
  immutable positive full24-bit span and primary-register flag callback. Name
  explicit admission results0/19/31/32/35; no callback on refused submission.
  Keep actual Legacy/backend0 and physical owner exclusion atomic. [x]

I21-003 Preserve Tom’s TX ISR stack and FIFO/CTS code; retain ownership until
  source release AND actual TEMT. Consumer additionally waits tagged module DONE
  before a netp epoch. No resident finish/queue/cancel or background game logic.
  Keep old experimental42/43 only in their original profile; never run both TX
  engines concurrently. Mainboard ordinary output routing remains unchanged. [x]

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


## Consumer and UART source review

MechAgon accepts this contract in net004-rfc25-consumer-acceptance-six; UART
owner’s rfc001-owner-policy-review-four confirms the early AF-only ISR boundary
and foreground TEMT retirement. A is the result and flags are unspecified;
BC/DE/HL/index/alternate registers/MB are preserved. Entry IFF is sampled before
DI. A scoped byte tracks the actual MOS executable dispatch mode and restores
its prior value after nested execution; MB0 is separately checked at entry.
Ordinary output/query traffic remains caller-serialized through source release,
TEMT and required receiver DONE. No new interrupt/finish service retires tail.


## Implementation cautions

The MOS API table contains16-bit targets. Slot25 therefore enters a low-ROM
trampoline in mos_api.asm, which jumps to the full24-bit optional owner address.
A direct table pointer to a high-ROM C/assembly unit would silently truncate.
The linked checker verifies this table/trampoline and the exact early TX branch,
retained RX saves, two FIFO bodies, tail guard and physical ROM size. It supplements
host admission checks; it does not claim CTS latency or hardware qualification.

## Source validation checkpoint — 2026-11-05

1. Actual admission C passes sanitizer checks for unsafe execution context,
   invalid/full24-bit spans, unavailable route and source/FIFO/physical ownership.
   Positive admission alone changes ownership.
2. All five inherited linked image guards and the new RFC25 guard pass. The
   new guard verifies actual low-ROM25 dispatch, early TX classification, retained
   RX saves, both FIFO bodies and bounded IER/TEMT ownership retirement. It is
   static linked proof, not runtime ISR/callback or CTS timing qualification.
3. Controller, netp and RFC25 share one ordinary-application span helper. The
   network local EX view reuses acknowledged NP fields rather than regenerating
   equivalent sequence/count/direction. No wire handshake or validation is lost.
   Netp TAKE clears upper output-size bytes once on entry; validated replies
   never exceed142.
4. The native verifier now stops at actual global ELF text symbols as well as
   C-style underscore labels. The adjacent RFC25 assembly entry exposed the
   previous naming assumption; the native control instruction guard is unchanged.
5. Actual C/P4 paired metadata/physical-owner checks pass0/1/127/128-byte payload
   cases in both directions, short-buffer no-consume and worker-confirmed close.
   Reused controller/client and network gateway sanitizer checks pass.
6. Unidentified linked image uses131007 bytes,65 free before identity strings.
   Clean identified candidate, runtime callback qualification and hardware remain
   pending. No firmware was installed; preserve installed component identities.
