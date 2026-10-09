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

## Development candidate

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
