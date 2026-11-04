# Eirene UART0 RC1 published — 2026-11-03

[Eirene MOS3.1.0 UART0 RC1](https://github.com/bgates747/eirene-mos/releases/tag/3.1.0_uart0_rc_1)
is published as a GitHub prerelease in the Author's fork. It is not marked
stable/latest and main remains upstream. Tag3.1.0_uart0_rc_1 points to frozen
source47dd56129c0043931bb32f7d329c0392a0b8a809 on codex/uart0-async, not main.
The main commit is still86b694260e3be3b391284ce4aa8ecc758f7f34e9.

The final Console8 run used stock Bistromathics VDP2.16.0 per the agreed
operator protocol. R0052/R0053 match the exact MOS identity and passed all16384
receiver payload bytes and functional checks. The Author reported completion
of the full procedure, including cold boot/example; the VDP/visual observations
are operator evidence, not independent machine fingerprints. See
[hardware evidence](hardware.md) and [raw reports](hardware/).

The release contains complete matching MOS and app sources, MOS binary/map,
original C example, no-argument fixture/comparator, manifests and tool identities,
licenses/credits, installation/restoration instructions and qualification data.
Firmware and app sources/binaries are unchanged from hardware qualification.
Only the unpublished draft's documentation/evidence/manifest were finalized.
Both uploaded asset digests were verified before publication and again after.

ZIP SHA256:
334726c7f2bccb6e002792e73636b93f2668885ddb34649ce106b405e6855318.
MOS CRC323F792F8D; SHA256
49873fb817f47cc76f50b8cec1c3d9cde7b252ac2ec844e08e745bdaf1c5eeab.
[Machine-readable publication and artifact pins](publication.json).

Pi and Oryx retain the final ZIP, source and release receipts at
build/tm001/rc1-final001. The returned card archive and report analysis remain
at build/tm001/rc1-hardware001; earlier draft/build history is preserved
separately. Card contents were inspected read-only, not changed. Next report54.
No emulator process ran for this RC step, per the Author's direction.

RC-001 through RC-005 are complete. This is a fork release candidate for aware
applications; upstream acceptance, a stable release, EMOS/other-board validation
and application/audio-deadline qualification are separate decisions. Existing
next application work remains the planned balls demonstration on the small
Eirene interface, not an extra gate imposed on this RC.
