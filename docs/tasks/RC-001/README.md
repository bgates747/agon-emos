# RC-001: Eirene MOS 3.1.0 UART0 RC1

Author authorized preparation on2026-11-03 after the complete five-MOS physical
comparison. Promote the small Tom-derived OTIRX implementation, not the larger
historical sender. Applications explicitly opt in through the new syscall;
ordinary startup and synchronous RST18 stay unchanged. Retain Eirene's existing
AgonDev toolchain. Do not modify the old official-MOS ZDS II project.

Release title: Eirene MOS 3.1.0 UART0 RC1. Proposed tag:3.1.0_uart0_rc_1.
Banner: Eirene UART0 MOS Version 3.1.0 RC 1.
Retain upstream major/minor/patch3.1.0; change its existing candidate type to RC
and candidate number to1. This is our fork's UART0 candidate, not an upstream
promotion of Tom's3.1.0_beta_1. Keep commits and the eventual tag on
codex/uart0-async. main stays at upstream86b694260e3be3b391284ce4aa8ecc758f7f34e9;
no merge or change of default branch is authorized by promotion.

RC-001 [ ] Freeze the selected implementation and publish the caller contract,
provenance and compatibility limits. Record the exact source commit. Keep
42h/43h fork-local, one borrowed source, bounded16-byte OTIRX, source-release
callback and explicit finish; no queue, copied RST18 or new recovery machinery.

RC-002 [ ] Set the agreed candidate branding using existing version fields.
Correct the inherited README build/toolchain instructions for Eirene. Keep
startup/reset/RST18/RX service source unchanged. Document board/VDP and API
qualification boundaries, installation, normal flash utility and recovery.

RC-003 [ ] Build the exact branded image on Oryx with the pinned AgonDev
compiler. Verify unchanged sender/startup source and object evidence against
the physical candidate. Run finite focused functional checks in Flexe when a
shared slot is available. Stage a no-argument test for manual Console8 cold
boot, ordinary output, async callback/NULL callback, finish and ordinary output
again. Author confirms the RC banner and runs the same fixture twice to check
normal return/re-entry. Do not claim physical qualification of new image bytes
until those reports are returned and checked.

RC-004 [ ] Prepare a self-contained versioned package: exact MOS binary/map and
source, build instructions/tool identities, checksums, original C example and
no-argument regression fixture with sources/binaries, calling contract,
credits/licenses, installation/recovery instructions and evidence. Archive the
returned old appliance before any SD replacement, retain sequential numbering
and use /test/mos-uart0 with concise readme.txt. Preserve history in project
archives rather than accumulating active SD variants.

RC-005 [ ] After the exact-image hardware check passes, publish a GitHub
prerelease in bgates747/eirene-mos from the frozen branch commit, with tag
3.1.0_uart0_rc_1 and pinned assets. A draft may be staged in advance. Do not
mark latest/stable, merge main or claim upstream acceptance. If the new image
needs changes after qualification, use a new frozen build and retest those
bytes rather than replacing a published artifact silently.

The previous physical evidence is
[TM-001 PERF2](../TM-001/hardware-perf2-r01.md):45trials and368640receiver bytes,
minimal OTIRX85.5% of no-traffic calculation rate during bulk sending and64.06%
more paced calculation than stock. Those results describe the existing
experimental build; the exact branded RC remains to be checked. Real apps,
EMOS fit, other boards and electrical CTS margins are outside that evidence.

Completion: all five tasks checked with immutable source/artifact pins and a
published prerelease. Physical flashing/execution on Console8 remain manual
Author actions; complete all independent build/package work before that step.
