# agon-emos

Agon Extender MOS (EMOS) is the project-owned, backward-compatible MOS
derivative that integrates the Agon Extender. It retains an upstream-shaped
MOS source tree so tagged official releases can be incorporated without
maintaining a second, structurally unrelated port.

The firmware behavior, resident EMOS service contract, implementation tasks,
tests, and qualification evidence are maintained here. Generic ZDS-to-AgonDev
translation, linking, runtime, emulator, and inspection infrastructure remains
in the separate `mos-agondev` project. See [OWNERSHIP.md](OWNERSHIP.md) for the
authoritative repository boundary.

The [mainboard SD foreground service](projects/sdserve/README.md) uses the
EMOS-owned `ext.sdlink` gateway to provide file access through the Extender
network endpoint while preserving native Extender keyboard input. It is a
foreground service, not a background file server during gameplay.

Draft v0.1.19 adds [foreground `/emos` utilities](docs/emos-utilities.md) and
retires cancelled external `.emo` provider loading. It passed the later scoped
physical deployment described in [AUDIT-008](docs/tasks/AUDIT-008.md), including
`/emos/sdserve.bin` dispatch and checked/fast transfers. That is not general
native-keyboard/gameplay acceptance or a release qualification.

For current product builds use the repository-root `make firmware-check` and
its selected EMOS source profile; see [ownership/build boundaries](OWNERSHIP.md).
The Extender [build guide](https://github.com/bgates747/agon-extender/blob/main/docs/building.md)
and [operation entry point](https://github.com/bgates747/agon-extender/blob/main/docs/using-extender.md)
cover the companion firmware and host clients.

## Documentation and lineage

The [current EMOS documentation](docs/README.md) contains resident interfaces,
foreground utilities and source ownership. It is the maintained reading path;
qualification tasks and research retain dated evidence separately.

EMOS derives from official Agon Platform MOS v3.0.2. Preserve upstream source
structure, notices and attribution. The imported upstream README remains in Git
history; its generic ZDS/emulator/root-SD recipes are not the maintained EMOS
build or recovery procedure. See [repository migration](docs/repository-migration.md)
for exact source lineage and [official documentation](https://agonplatform.github.io/agon-docs/)
for the stock MOS surface.

## Licensing

The MOS-derived code is MIT licensed, subject to individual component notices.
The FatFS license is preserved in [src_fatfs/LICENSE](src_fatfs/LICENSE).
Retain all inherited notices when incorporating or distributing changes.
