# EMOS v1 provisional contract

Status: provisional v1 byte layouts frozen by `EMOS-102` on 2026-08-24. This
document states decisions and non-goals. It is not an actionable checklist;
`TODO.md` is the only task list.

## Product boundary

EMOS v1 is one backward-compatible MOS firmware image built from maintained
ZDS-shaped source and carried through this repository's existing conversion and
AgonDev qualification pipeline. Core MOS owns discovery, registry publication,
module-area loading, every supported gateway, VDU routing, mode commitment,
canonical VDP sysvars, and all eZ80-memory writes. External providers receive
bounded requests; they do not become a second operating-system authority.

The upstream MOS Modules document remains a proposal. EMOS therefore uses a
local ABI with an explicit version and a narrow Core gateway. Module binaries,
manifests, and calls fail on an unknown major version. Upstream adoption can
replace the loader and translate requests behind that gateway without changing
ordinary Core MOS entry points.

## Selected v1 behavior

1. External modules are ADL binaries linked for the existing 32 KiB moslet area
   at `0x0B0000`. A fixed, versioned header describes file length, entry offset,
   Core ABI range, provider class and identity, capability flags, and CRC-32.
   Discovery reads and validates metadata without executing code.
2. `EMOS$Path` names the configured module directory. Discovery occurs only on
   an explicit `*EMOS DISCOVER` or equivalent gateway request, never merely
   because hardware is present or at early boot. Entries are sorted by
   class/namespace/name before transactional publication, so FatFS enumeration
   order cannot change the result. Any duplicate claim fails the whole new
   registry and preserves the old one.
3. V1 external providers are synchronous and transient. Core loads exactly one
   selected provider, validates it again, invokes one fixed entry with a
   versioned request block, and considers all provider code/data pointers dead
   when the call returns. The Core gateway address is stable; transient entry
   addresses are never exposed through `mos_getfunction` or public handles.
4. Nested module calls, reentrant dispatch, interrupt entry into module code,
   asynchronous callbacks, external persistent residents, dependencies between
   external modules, and provider-controlled teardown are rejected. A future
   loader may add them without weakening the v1 call boundary. Persistent or
   asynchronous transport belongs in statically linked Core adapters until a
   separately qualified resident-memory design exists.
5. Provider classes in v1 are star commands and namespaced services. Existing
   built-ins and aliases win before module commands. A versioned MOS API and a
   non-colliding C-function slot reach the same Core service gateway. New
   project-owned operations use the `edu.*` namespace. No transient pointer is
   a public API result.
6. A module-safe advanced-header ADL program may use transient providers. A
   module-compatible ADL program may do so only through a Core-controlled,
   transactional save/replace/restore of the entire 32 KiB module area and only
   when request buffers do not overlap it. Unheadered, version-0, malformed,
   Z80, unaware, and moslet callers receive a deterministic unavailable/unsafe
   result for module-backed facilities. Core-only MOS calls continue to work.
   The compatible path requires writable FatFS storage and reserves
   `/.emos-swap.bin` as a private synchronized preservation file. A failed save
   is removed before the area changes. A failed restore retains that file and
   blocks implicit replay while the application remains live; a later call may
   retry restoration, and application exit discards state that no longer has a
   live owner. Cold boot removes any orphan from an interrupted prior session.
7. RST 10, RST 18, and C-runtime output call one semantic VDU dispatcher. Each
   invocation snapshots one committed backend. Raw UART APIs keep their public
   identities. Legacy and Dual use only onboard VDP for ordinary VDU; exclusive
   modes use only EDP. Ordinary output is never mirrored.
8. The mode coordinator stores ordinary-VDU route and EDP-service state as two
   committed planes whose only public combinations are Legacy, Dual, Exclusive
   Compatible, and Exclusive Extended. Legacy is the transition hub. Every
   non-Legacy request performs prepare/readiness/commit/recover and publishes
   only after all actors are ready. With no qualified EDP adapter, activation
   fails boundedly and leaves Legacy fully operational.
   Production cold boot selects an unavailable adapter. The explicit
   `*EMOS FAKE ON` test seam supports only Dual: it marks a separate fake EDU
   result domain active while ordinary VDU remains onboard. It deliberately
   rejects both exclusive modes because it supplies no physical VDU backend.
   `*EMOS FAKE OFF` is accepted only in Legacy. This fake is qualification
   scaffolding, not EDP discovery or transport.
9. The generic example proves discovery and synchronous command/service
   dispatch. The Extender-shaped fake proves `edu.*` negotiation and separate
   result ownership but is not a physical EDP backend. Its data never updates
   canonical stock VDP sysvars.
10. All container and manifest output is generated and validated by one
    deterministic host tool. The target and host share mechanically checked
    magic values, versions, sizes, limits, flags, and status codes. Negative
    fixtures are first-class contract evidence.

## Decision inventory

| Decision | Rationale | Rejected alternative | Compatibility effect | Replacement seam | Required evidence |
| --- | --- | --- | --- | --- | --- |
| Fixed-address transient image | Matches upstream direction and existing 32 KiB area; avoids an invented relocator | Persistent arbitrary binaries or ad hoc relocation records | Existing moslets remain unchanged; eligibility gates prevent overwrite | Container loader behind Core gateway | Boundary, size, CRC, reload, and stale-pointer tests |
| Transactional sorted registry | Enumeration order must not choose providers or partial state | First file wins | Built-ins/aliases retain precedence; duplicates are explicit failures | Registry builder/publication interface | Reordered-media and collision fixtures |
| One request-block gateway | Versionable, testable, and prevents public transient pointers | Direct per-module function pointers | Adds APIs without renumbering existing calls | MOS API/C-function wrappers | Assembly and C ABI probes |
| Transient-only external v1 | The module area cannot safely host asynchronous Extender state | Pretend swappable code can own interrupts/callbacks | Unsupported capabilities fail, rather than corrupting state | Statically linked resident adapter table | Flag rejection and reentrancy tests |
| Full-area disk preservation for compatible apps | Implements the documented meaning instead of silently treating compatible as safe | Treat compatible as safe or deny it forever | Requires writable storage and non-overlapping buffers | Save/restore provider in Core | Atomicity, overlap, storage, and restore-failure tests |
| Fixed VDU dispatcher | Required by accepted Extender architecture; one authority and snapshot per call | Vector swapping or mirroring | Legacy byte behavior remains the default | Backend table and mode coordinator | Linked-path audit and Legacy transcript parity |
| Fail-closed physical EDP adapters | Wiring and reverse protocols remain unresolved | Invent a production wire contract | Stock behavior stays operational in Legacy | Adapter table selected by later accepted work | Failed activation recovery and fake-only tests |

## Explicit non-goals

1. EMOS v1 does not implement EDP firmware, carrier wiring, power sequencing,
   physical discovery signaling, final enhanced transport, or a qualified
   Exclusive Compatible UART circuit.
2. It does not claim that the local container, request ABI, API number, or
   C-function number is accepted upstream or stable beyond its declared major
   version.
3. It does not relocate arbitrary object files, dynamically link symbols, or
   support multiple simultaneously resident external modules.
4. It does not permit external providers to own interrupts, callbacks, queues,
   canonical VDP sysvars, VDP response parsing, mode state, or direct route
   changes.
5. It does not provide nested or reentrant external-module calls, external
   dependencies, hot replacement during a call, or asynchronous teardown.
6. It does not make unheadered programs, version-0 executables, Z80 programs,
   or moslets module-safe by inference.
7. It does not preserve live graphical, audio, input, RTC, or VDP parser state
   across non-Legacy mode changes. V1 may require a documented controlled
   restart; state-preserving transitions remain v2 work.
8. It does not settle unresolved D003 through D008 Extender policy. Fake seams
   are labelled and replaceable, and their results remain outside stock sysvars.
9. It does not alter raw UART APIs or promise to police arbitrary machine code
   that directly accesses hardware or memory.
10. Emulator evidence will not be presented as electrical, SD-timing,
    power-order, or physical-hardware qualification.

## Compatibility and failure posture

Legacy is the cold-boot default and remains fully usable when no module
directory, provider, Extender, or writable preservation storage exists. No
automatic discovery runs before normal boot is complete. Registry replacement
and mode activation are prepare-then-commit operations; failure retains a known
stable state. Malformed lengths, offsets, flags, versions, identities, CRCs,
collisions, oversized images, unavailable adapters, active-call replacement,
and unsafe callers receive bounded errors rather than partial activation.

The design cannot protect against arbitrary privileged eZ80 machine code.
Safety statements apply to EMOS firmware, its supported gateways, shipped
providers, tools, and examples only.
