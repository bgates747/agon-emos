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

[TODO](../TODO.md) owns unfinished EMOS work. Task/qualification records and
`research/` preserve evidence for their recorded revisions, not alternative
current instructions. The [migration record](repository-migration.md) explains
repository lineage; old provider-module qualification does not qualify current
foreground utilities. Current limits are stated in the contracts above, so
routine readers do not need to reconstruct them from historical tasks.
