# EMOS documentation

Current interfaces and operating rules:

1. [Resident service contract](emos-v1-contract.md): gateway ABI, EMOS ownership,
   SD transport and diagnostic service limits.
2. [Foreground utilities](emos-utilities.md): `EMOS <utility>`, `/emos` placement,
   executable bounds, caller admission and return behavior.
3. [SD listener](../projects/sdserve/README.md): build layouts and current invocation;
   the linked Extender guide owns host commands, fast mode and recovery.
4. [Repository ownership](../OWNERSHIP.md): maintained source versus generic
   AgonDev tooling, official references and the working MOS fork.
5. [Extender handbook](https://github.com/bgates747/agon-extender/blob/main/docs/README.md):
   companion P4 firmware, bench operation, builds, SD layout and recovery.
6. [Tool index](../scripts/README.md): build preparation versus retained emulator
   helpers; historical provider-media tools are not EMOSlet installers.

[TODO](../TODO.md) owns unfinished EMOS work. Task/qualification records and
`research/` preserve evidence for their recorded revisions, not alternative
current instructions. The [migration record](repository-migration.md) explains
repository lineage; old provider-module qualification does not qualify current
foreground utilities. Current limits are stated in the contracts above, so
routine readers do not need to reconstruct them from historical tasks.

## Experimental ordinary ExExt

`port/exext-stream-candidate.mk` builds the new ordinary byte-stream binding:
EMOS batches existing application VDU and services actual return packets on the
real VBlank handler, retaining bundled return bytes across ticks so each keyboard
event gives the foreground a chance to observe stock MOS's latest-event latch.
The obsolete indefinite forward epoch is excluded only in
this explicit composition. Software/full-ROM checks and bounded physical
CLI, reply, cached-input and finite-game checks pass. However, the 2026-10-11
experiment shows a major gameplay-performance regression and unreliable reset
admission; it is not accepted for ordinary use. The bench rollback restores the
preceding verified UART pair; the experimental source remains available only
through explicit matched candidate builds. See the [paired work contract](../../agon-extender/docs/tasks/PORT-008/EXEXT-STREAM-R01.md)
and [wire/scheduling contract](../../agon-extender/docs/protocols/exext-stream.md).
The [completion-removal cut](../../agon-extender/docs/tasks/PORT-008/COMPLETION-REMOVAL-R01-RESULTS.md)
now compiles CRC-free format4 on both processors, with no software COMPLETE/ACK.
Native completion, DMA cleanup and reciprocal UART return establish completion;
P4 retains completed storage until its consumer takes it. Formats2/3 cannot admit.
Former CRC slots remain ignored zero padding; version1 activation retains CRC.
Full builds and offline owner/linked checks pass. ExExt EMOS fits at 130,334
bytes/738 free (another 264 bytes reclaimed); ordinary 128,482 bytes/2,590 free.
These exploratory builds are uninstalled, without a physical performance pass.
No hardware changes while fsim uses the bench. Legacy/ExCom and selected
production are unchanged. Following older sizes/activation statements describe
their named checkpoints, not this build.

## Development candidate

The separate `port/controller-candidate.mk` builds the experimental
[controller read service](tasks/INPUT-001.md) for Extender's mode20 viewer. It
uses the existing resident gateway and EMOS-owned UART, with cached READ and
bounded REQUEST operations. Native service/pipeline and full-ROM/linked checks
pass; no physical controller, UART, display or coexistence qualification is
claimed. Default firmware selection excludes it. The Author-approved v0.1.26
correction is retained by installed v0.1.27 with complete ROM readback; ordinary selection still
excludes it. The earlier viewer review lost controller and keyboard input.
That masked-read UART failure is reproduced by a linked instruction test;
the correction removes controller CRCs, returns busy during a pending refresh
and copies96-byte snapshots with interrupts enabled. Native/linked checks and
the identified full-ROM build pass. P4r13, sdserve-v0.3.0 and viewer-r03 are
installed. The Author reported successful controller operation and deferred
latency work; broader controller qualification remains open.
The profile also retains explicit unversioned builds.

The Author-approved v0.1.27 extension retains that controller composition and
adds only bounded mainboard `VDPL` replies alongside `P3DR` to the existing
callback-only path. Its identified full build fits at 130802 bytes/270 free and
passes all mandatory linked checks. Installation/full-ROM verification and
finite physical discovery/Escape checks in modes0/20 pass. The selected
production bundle is unchanged. See the maintained
[completion contract](emos-v1-contract.md#pingo-completion-compatibility-candidate)
and [implementation evidence](tasks/INTEG-015.md).

[REMOTE-005](tasks/REMOTE-005.md) adds private idle-CLI admission and finite utility
dispatch. It is not an available automatic file-transfer service or production
release. Continue using the documented manual listener until paired qualification.

PORT-008/INTEG-016 also has a private native parallel coordinator binding the
shared boot handover, UART reservation/parking, native assembly and matched
completion status. The complete `parallel-native-candidate.mk` composition
uses 130473 ROM bytes /599 free. Software/build checks pass; the coordinator
has no activation caller or public ExExt API/mode and is not hardware-qualified.
Ordinary EMOS uses 128739 /2333 free. UARTTEST and VDPPOLL now load from SD,
using the existing admitted diagnostic service; resident text services remain.
See the [probe MOSlet guide](../projects/uartprobe/README.md) and paired
[recovery results](../../agon-extender/docs/tasks/PORT-008/DIAGNOSTIC-ROM-RECOVERY-RESULTS.md).
Complete wrappers and mandatory link guards pass. No physical activation caller or
hardware qualification is supplied; these private bytes have not been deployed
or selected for production. The separate RAM instruction-test image is not
firmware. Earlier overflowing/fitting checkpoints remain historical evidence.

The maintained C reference is selected only by
`port/parallel-reference-test.mk` (`EMOS_PARALLEL_RECEIVE_REFERENCE=1`), for
paired host and linked eZ80 correctness tests. Select that profile through the
configured mos-agondev repository-root `prepare-mos` and `firmware-check`
targets, preserving all profile checks. Ordinary/native profiles exclude it.
The native profile has a strict private owner checker; ordinary profiles reject
its assembly leaf. Supported operations and installed firmware are unchanged.
See [INTEG-016](tasks/INTEG-016.md) for evidence and remaining integration gates.


The next private qualification composition, `port/parallel-probe-candidate.mk`,
adds a sole admitted diagnostic MOSlet caller. Full builds and modeled/linked
checks pass at 130984 ROM bytes /88 free; unactivated native and ordinary sizes
above are unchanged. [parprobe](../projects/parprobe/README.md) owns patterns,
checks and durable reporting for twenty two-way blocks, while EMOS retains
negotiation, reservation and physical coordination. This service is absent from
ordinary firmware, has not been installed, and adds no formal ExExt mode/API or
SD transport. The matching P4 compile image has no installable identity.
[Current paired results](../../agon-extender/docs/tasks/PORT-008/LIVE-QUALIFICATION-RESULTS.md)
own the review boundary before physical installation; previous unactivated
checkpoint statements describe LC02, not this private caller.
