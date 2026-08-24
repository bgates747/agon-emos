# EMOS prior-art digest — 2026-08-24

This dated digest records the bounded research used to select the provisional
EMOS v1 boundary. It is evidence, not an actionable task list or an upstream
specification.

## Current Agon sources

1. Official `AgonPlatform/agon-mos` still resolves `main` and release `v3.0.2`
   to `8336409351ee5314e02801a7b72a4f1bb5282519`. No later tagged MOS source was
   selected.
2. The current official [MOS Modules proposal](https://github.com/AgonPlatform/agon-docs/blob/main/docs/mos/Modules.md)
   still says MOS 3.0 has no module system and leaves the exact design open. It
   predicts the 32 KiB moslet area, Core MOS, and command/API/C-function
   providers, but supplies no container, registry, loader, lifecycle, or ABI.
3. The current official [executable documentation](https://github.com/AgonPlatform/agon-docs/blob/main/docs/mos/Executables.md)
   defines advanced header version 1 at file offset `0x40`: type at `0x44`,
   flags plus inverse at `0x45`/`0x46`, and optional load address at
   `0x47`-`0x49`. Bit 0 means module-safe and bit 1 means module-compatible;
   MOS 3.0 documents but does not implement those semantics.
4. Official issue [AgonPlatform/agon-mos#2](https://github.com/AgonPlatform/agon-mos/issues/2)
   remains open under the MOS 3.1 milestone. Its public discussion confirms the
   transient module-area direction but does not provide an implementation
   contract. A search of current official code, pull requests, branches, and
   public GitHub code found no later usable module loader or registry.
5. The only nearby implemented extension seam is `mos_getfunction` from merged
   [pull request #169](https://github.com/AgonPlatform/agon-mos/pull/169). MOS
   v3.0.2 exposes C-function slots `0x00` through `0x11`; MOS API `0x51` and
   above remain unimplemented. These are evidence of vocabulary, not an EMOS
   number assignment from upstream.

## Transferable constrained-system lessons

1. BBC Micro sideways-ROM priority shows why implicit first-provider wins is a
   policy, not a neutral implementation detail. EMOS instead sorts claims and
   rejects collisions transactionally. The reviewed primary Acorn material is
   archived by MDFS in the [ARM System User Guide](https://mdfs.net/Docs/Books/ARMCoPro/ARMSystem.pdf).
2. CP/M 3 Resident System Extensions expose a named/numbered system-call seam
   with a deterministic not-resident result. The transferable lesson is a
   stable Core gateway and explicit unavailable status, not adoption of CP/M's
   binary layout. The reviewed call summary is the [CP/M BDOS system-call archive](https://www.seasip.info/Cpm/bdos.html).
3. RISC OS-style resident module systems and general dynamic linkers solve a
   larger problem than EMOS v1 can safely own in one swappable 32 KiB region.
   Relocation, arbitrary symbol resolution, interrupt residents, and nested
   service calls are intentionally deferred rather than incompletely copied.

## Resulting inference

Because no current upstream implementation fixes the missing binary and
lifecycle contracts, EMOS must choose a local, versioned, replaceable seam. A
fixed-address transient image, deterministic transactional registry, and one
stable Core request gateway provide the smallest system that can prove real
discovery and dispatch while failing closed for persistent/asynchronous
Extender needs. Statically linked Core adapters remain the only v1 location for
such persistent ownership.
