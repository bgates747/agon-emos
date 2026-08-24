# Repository ownership

This repository is the canonical home of Extender MOS (EMOS). The repository
boundary is part of the product architecture and must not be bypassed merely
for development convenience.

## This repository owns

1. The complete maintained, upstream-shaped MOS source used to build EMOS.
2. EMOS behavior, APIs, dispatch, module/service ABI, mode coordination, and
   eZ80-side Extender integration.
3. EMOS-specific source tests, module/container tools, provider fixtures,
   qualification procedures, evidence, release metadata, tasks, and research.
4. The process for incorporating later tagged official MOS releases into the
   EMOS lineage.

## Neighboring repositories

1. `agon-mos` is the Author's upstream-oriented official-MOS fork. Generic MOS
   fixes may live there, but EMOS product behavior, branches, and task history
   do not.
2. `mos-agondev` owns reusable preparation, ZDS-to-GNU-as translation,
   AgonDev compilation and linking, restricted runtime support, emulator setup,
   generic ABI probes, and generic qualification infrastructure. Generated
   prepared source, assembly, objects, maps, and firmware are disposable
   outputs, never maintained source.
3. `agon-extender` owns the complete Extender product architecture: EDP/EMOS
   requirements, operating modes, hardware, physical transports, and
   cross-component qualification. It may constrain EMOS but does not own this
   firmware's implementation details or task queue.

## Change routing

1. A change to what EMOS does belongs here.
2. A generally useful stock-MOS correction should first be isolated in
   `agon-mos`; EMOS then incorporates that clean source commit or an equivalent
   tagged upstream release.
3. A change needed to translate, compile, link, emulate, or inspect arbitrary
   supported MOS-family source belongs in `mos-agondev`.
4. A requirement spanning the eZ80, EDP/P4 firmware, onboard VDP, wiring, or
   assembled product belongs in `agon-extender`, with an implementation task
   here only for the EMOS-owned portion.
5. Candidate-specific exceptions must not be hidden inside generic porting
   tools. Express them through an explicit source profile or a documented,
   fail-closed interface.

## Build boundary

EMOS remains ZDS-oriented maintained source. ZDS II may build it directly.
AgonDev builds use a reviewed `mos-agondev` revision configured with this
checkout as its maintained-source input. Never edit `mos-agondev`'s generated
worktree or generated GNU-as output to change EMOS behavior. The tracked
`port/mos-agondev.mk` profile declares EMOS's additional source unit, runtime
object, and reviewed command-list difference without transferring their
ownership to the generic port. Machine-local checkout, toolchain, and emulator
locations are supplied through `MOS_AGONDEV_ROOT`, `AGONDEV_TOOLCHAIN`, and
`FAB_ROOT`; they do not belong in tracked product metadata.
