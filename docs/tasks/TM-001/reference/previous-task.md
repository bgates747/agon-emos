# TM-001: compare Tom Morton's small UART sender

The Author supplied Tom's candidate on 2026-11-03 and requested an honest code
comparison, a search for unnecessary complexity in our implementation, and the
same benchmarks for his sender. A much smaller single-transfer helper could be
useful even if it does not replace every service currently provided by MOS.

TM-001 [x] Preserve the exact attachment and identify its narrower contract.
TM-002 [x] Audit both implementations; separate useful safety from optional
features, dead code, redundant state and avoidable work in the hot path.
TM-003 [x] Build a separate AgonDev benchmark app comparing standard output,
our batch16 borrowed/direct path and Tom's scalar sender on identical bytes.
Preserve Tom's algorithm; record syntax/ABI changes and repair only the sample
caller's completion-arm race, not the ISR being measured. No firmware changes.
TM-004 [ ] Run finite actual-firmware Flexe checks on both engines through the
shared Oryx queue. Receiver verification, stock CLI return and timeout evidence
are prerequisites for considering timing. Retain failures; do not improve Tom's
code silently. Emulator time does not establish physical CPU savings.
TM-005 [ ] Prepare repeatable paced/useful-work and bulk comparisons for manual
Console8 use, using the existing branded ZDS II MOS and matched VDP; no flash or
SD deployment until an appropriate card/target is available. Physical timing is
the authority for speed/headroom claims.
TM-006 [ ] Recommend a smaller delivery design based on evidence; preserve the
existing qualified candidate while evaluating changes. Do not break consumer
contracts merely to reduce line count or present optional copied RST18 as part
of the minimal borrowed-transfer requirement.

Original: experiments/uart0-tomm/original/async_vdu.asm, SHA256
93e5ddad8e72f1b22541c16e15cd0600e820204f02e13a559c2a6f6b423f5bd4.
Provenance is the Author's attachment, not a fetched or attributed upstream release.
Tom says OTIRX is not yet implemented. No third-party license is asserted by us.

## Current evidence

Source findings: [comparison.md](comparison.md). The draft cleanup-proposal.patch
removes25lines (dead copy path, one redundant lookup and dead flag setup) without
altering maintained firmware. An isolated ZDS II/Wine build links successfully and saves53ROM bytes.
Functional requalification is still pending.

AgonDev run001 built compare.bin (25126B). Six stopped private profiles are
staged on Oryx under /home/smith/Agon/mystuff/agon-mos/build/uart0-tomm/run001.
Shared FIFO request c469951ff727 is behind the current Wolf3d human review and
agon-6502's earlier test. No emulator has been launched for TM-001 yet.
