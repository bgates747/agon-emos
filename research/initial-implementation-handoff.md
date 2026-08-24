# EMOS modular-service implementation handoff

## Purpose

This handoff prepares a fresh agent to design and, when its invocation explicitly
authorizes implementation, build the first coherent **Extender MOS (EMOS)**
modular-service architecture. The Author may run that agent unattended. The
agent must therefore make a detailed bounded task list before editing, refer to
it continuously, record every invented contract, and resist unrelated MOS,
Extender, toolchain, emulator, or hardware work.

This document does not override `AGENTS.md`, the emulator human-approval gate,
or an invocation that requests research or planning only.

## Mandatory first reading

Read these completely, in this order, before searching or changing code:

1. [`AGENTS.md`](AGENTS.md), then every document under
   [`../agon-dev-env/codex/`](../agon-dev-env/codex/), completely. The shared
   directory is a required session-start corpus, not a menu of optional
   references; in particular its emulator and bespoke-VDP guides own profile
   launch, host-library, custom-MOS, native-module, and human-gate policy.
2. [`../agon-extender/agents/precis/mos-modules.md`](../agon-extender/agents/precis/mos-modules.md)
   — public history, documented direction, implemented groundwork, repository
   survey, and unresolved questions for the proposed upstream MOS Modules
   system.
3. [`../agon-extender/agents/precis/mos-sysvars.md`](../agon-extender/agents/precis/mos-sysvars.md)
   — exact current ownership and data flow for MOS sysvars, VDP return packets,
   and completion flags.
4. [`STARTHERE.md`](STARTHERE.md), [`README.md`](README.md),
   [`docs/README.md`](docs/README.md), and [`TODO.md`](TODO.md) in this
   repository.
5. The accepted Extender architecture and its still-open boundaries:
   - [`../agon-extender/docs/architecture.md`](../agon-extender/docs/architecture.md);
   - [`../agon-extender/docs/decisions/ADR-0014-edu-operating-modes-and-service-architecture.md`](../agon-extender/docs/decisions/ADR-0014-edu-operating-modes-and-service-architecture.md);
   - [`../agon-extender/docs/decisions/AUDIT-2026-08-23-001-operating-mode-semantics.md`](../agon-extender/docs/decisions/AUDIT-2026-08-23-001-operating-mode-semantics.md);
   - [`../agon-extender/docs/tasks/REMED-001.md`](../agon-extender/docs/tasks/REMED-001.md)
     and its four accepted analysis files under `docs/tasks/REMED-001/`;
   - [`../agon-extender/docs/tasks/SETUP-005.md`](../agon-extender/docs/tasks/SETUP-005.md);
     and
   - [`../agon-extender/docs/tasks/MOS-001.md`](../agon-extender/docs/tasks/MOS-001.md).

Do not substitute summaries for those readings. Distinguish accepted
architecture, provisional recommendations, unresolved task questions,
historical evidence, and generated candidate data.

## Frozen Extender reference

The initial EMOS work is constrained by the frozen `agon-extender` commit
`10f2eb20997f0428f0d756a0819a632cd3b7cedb` (`Record ADMIN-001 closure and
prototype-led D003 sequencing`). Treat the Extender documents at that exact
commit as the architectural reference for this implementation. Do not silently
adopt later Extender changes or reinterpret the accepted contract from a moving
checkout. A later Extender revision becomes authoritative for EMOS only when
the Author explicitly selects and records a new reference commit.

## Correct development topology

The existing pipeline has already carried one complete MOS baseline from its
maintained ZDS-shaped source through conversion, AgonDev compilation, linking,
boot, and qualification. That proves the pipeline; it does **not** turn the
resulting AgonDev/GNU-as files into maintained source and does not make porting
a finished one-time event.

EMOS development continues the same repeatable porting process for both kinds
of source change:

1. a newly selected tagged upstream MOS release incorporated into the EMOS
   source lineage; and
2. an EMOS change made in that ZDS-shaped lineage.

For either kind, begin with maintained ZDS-oriented source, run it through the
existing preparation and conversion tools, inspect the conversion and its
generated differences, reconcile any new source-language or portability issue
in the durable tooling, and only then compile and qualify the final AgonDev
port. Agent inspection is an explicit stage of the port: never assume that a
successful textual conversion proves semantic equivalence, and never conceal a
new ZDS construct with an ad hoc generated-file edit.

Do not start a second independent ZDS-to-AgonDev port or replace the established
pipeline. Extend the existing converter, preparation rules, tests, and contracts
when an upstream or EMOS source update exposes a real gap.

The two repositories have different authorities:

1. [`../agon-mos`](../agon-mos) is the Author's fork of official MOS. Its
   canonical `agon-emos` `main` is the maintained ZDS-shaped source lineage for
   EMOS. The former `agon-mos` `dev/emos` branch was migration input only.
2. This `mos-agondev` repository owns translation, AgonDev compilation,
   linking, runtime integration, generated-worktree preparation, emulator
   setup, regression fixtures, and qualification infrastructure.

Follow the maintained-source workflow in `STARTHERE.md`: edit ZDS-oriented MOS
source in the writable `agon-mos` worktree; never hand-edit generated GNU-as or
prepared MOS output. Firmware behavior and upstream-release integration belong
in that source lineage. Change this repository when the continuing port needs a
durable translator, preparation, build, linker, runtime, fixture, inspection,
or test-harness capability.

The writable fork is therefore not redundant and this repository is not a
replacement source fork. They are the maintained input and the conversion,
build, inspection, and qualification environment, respectively.

## Mission

Implement a versioned, testable first EMOS modular-service system informed by
the upstream **MOS Modules** proposal but intentionally weighted toward the
accepted Extender requirements below. Where upstream has not specified a
contract, choose a coherent provisional EMOS contract, explain the choice,
isolate it behind replaceable boundaries, and label it as local rather than
pretending it is upstream policy.

The desired result is not merely a design essay. When implementation is
authorized, produce an internally complete vertical system that can be built,
booted, exercised, fault-tested, and inspected in the emulator. “Complete”
means complete for its declared first-version contract, not an unsupported
claim that the ABI is final forever or already accepted upstream.

Do not attempt to implement the entire EDP firmware, settle unresolved physical
wiring, or invent the final Extended wire protocol as incidental work. Use
bounded fake, loopback, or example services where the modular substrate needs
an Extender-shaped consumer before the real endpoint exists.

## Prime compatibility and ownership requirements

1. **EMOS definition.** EMOS is one complete backward-compatible replacement
   build of official stock MOS. It is not a companion running beside stock MOS.
   It must retain ordinary stock MOS behavior and interfaces except where an
   accepted Extender requirement explicitly changes routing or adds a service.
2. **Existing vocabulary survives.** Existing star commands, MOS APIs,
   `RST.LIL 10h`, `RST.LIL 18h`, C-runtime output calls, executable formats, and
   public status behavior remain recognizable. Optional implementation behind
   those entry points may become modular; applications should not need an
   entirely new vocabulary merely because EMOS loads a provider.
3. **One supported authority.** EMOS alone owns ordinary VDU routing, Extender
   activation, transport authorization, committed operating mode, canonical
   MOS VDP sysvars, completion flags, and every eZ80-memory update. Project
   modules, linked application bindings, resident services, examples, and tests
   must expose no supported bypass.
4. **No absolute safety claim.** The project enforces the ownership contract in
   its own firmware, APIs, tools, examples, and documentation. Arbitrary eZ80
   machine code can still manipulate peripherals or memory directly and may
   corrupt or brick a system; that unsupported behavior is caveat emptor.
5. **Conservative unaware-program behavior.** Programs lacking new metadata or
   explicit service negotiation must not be silently treated as safe. Preserve
   stock behavior when possible, deny unavailable module-backed facilities
   deterministically when necessary, and never overwrite an unaware program's
   moslet-area code or data merely to satisfy a request.
6. **Declared aware-program behavior.** Implement and test a coherent meaning
   for the documented module-safe and module-compatible executable flags. If
   module-compatible save/replace/restore is selected, define its atomicity,
   buffer-overlap restrictions, failure recovery, and storage assumptions.
7. **Upstream-shaped source.** Keep official names, source placement, APIs, and
   implementation recognizable wherever practical. Local changes should be
   narrow, heavily explained when unusual, and designed for reviewing and
   incorporating future tagged upstream MOS releases.

## Extender operating-mode contract that constrains EMOS

The formal modes are:

| Formal mode | EMOS ordinary-VDU route | EDP service | Canonical stock state |
| --- | --- | --- | --- |
| Legacy | onboard VDP | inactive | onboard VDP through the stock MOS parser |
| Dual | onboard VDP | active through EDU | onboard VDP; EDP results are separate |
| Exclusive Compatible | EDP over the stock-compatible UART contract | active | EDP responses through the EMOS/MOS parser |
| Exclusive Extended | EDP over parallel-forward/enhanced-UART-return transport | active | EDP responses through the EMOS/MOS parser |

“Compatible” and “Extended” are acceptable short forms; the formal exclusive
names include “Exclusive.”

The following accepted rules are non-negotiable inputs:

1. EMOS owns two committed planes: ordinary-VDU route and EDP-service state.
   Their four valid combinations are exactly the four modes above. An exclusive
   route with inactive EDP is invalid.
2. Every cold boot reaches fully operational Legacy. Merely attaching, powering,
   or resetting Extender does not contact it or change mode.
3. EDP presence is pull-discovered only after an explicit late `autoexec.txt`
   command or manual EMOS invocation. EDP firmware and carrier wiring remain
   quiescent before EMOS prepares a receiver and opens a bounded activation
   transaction.
4. Legacy is the transition hub. No direct contracted transition occurs between
   two non-Legacy modes.
5. Activation is prepare/readiness/commit/recover. EMOS validates and prepares
   every actor before atomically publishing a new route or active service.
   Pre-commit failure leaves or restores a stable mode and produces one bounded
   result for one explicit request.
6. EMOS uses one fixed set of modified VDU restart and C-runtime handlers calling
   one semantic dispatcher. A mode transition switches its committed backend;
   it does not install a different reset-vector table for each mode.
7. Each active dispatcher invocation snapshots one backend. EMOS must not
   switch it in the middle of an invocation, partial command, owned response,
   or callback/event transaction.
8. Ordinary VDU is never mirrored. Legacy and Dual send it only to onboard VDP;
   both exclusive modes send it only to EDP. Private input/bootstrap,
   diagnostics, and explicitly labelled qualification traffic are targeted
   exceptions, not duplicated application output.
9. `RST.LIL 10h`, `RST.LIL 18h`, and corresponding C-runtime output remain VDU
   calls. EMOS selects their destination. Explicit EDU calls use a separate,
   versioned application interface and separate result domain even when both
   interfaces reach EDP in an exclusive mode.
10. Rev 1 defines no direct VDP/EDP communication. The eZ80 running EMOS is the
    intermediary. A later bidirectional link is aspirational and outside this
    implementation.
11. Stock MOS supports only Legacy. All active Extender modes require EMOS.
12. Proof-of-concept and beta route changes may use a disruptive controlled
    restart. State-preserving transitions are aspirational for v1 and required
    for v2; do not smuggle them into the module substrate.
13. No current harness qualifies Exclusive Compatible's stock physical UART
    implementation. Firmware requirements precede a separate hardware design
    review and qualification. Do not infer that wiring contract from the
    enhanced split-link prototype.

## Sysvar and response-domain requirements

Use the sysvars précis as factual authority for current MOS behavior.

1. MOS/eZ80 owns and allocates canonical sysvars. Neither VDP nor EDP writes
   eZ80 RAM directly.
2. In Legacy and Dual, the onboard VDP sends stock UART response packets and
   the MOS parser updates canonical sysvars and completion flags.
3. In either exclusive mode, EDP must reproduce required stock-visible response
   semantics; EMOS/MOS remains the only code that parses those responses into
   canonical sysvars.
4. In Dual, EDP EDU results, events, callbacks, and state remain in a separately
   identified EDU-owned domain. They do not compete for canonical VDP sysvars.
5. Do not mirror response streams, guess a producer from packet bytes, or allow
   two producers to write one canonical result category.
6. Current sysvars are latest-state storage and category notification, not a
   queued, source-labelled, request-correlated response system. Any modular
   service needing stronger semantics must use a distinct versioned mechanism
   rather than silently changing the stock ABI.

SETUP-005 deliberately leaves some response, Dual behavior, audio, input, RTC,
maintenance, and enhanced-reverse details unresolved. Do not resolve them by
accident inside a generic module ABI. Assign provisional choices to explicit
experimental seams and record the downstream question they touch.

## Modular-service outcomes

The first declared EMOS module-system contract should provide and test:

1. a versioned module identity, metadata, compatibility, and integrity model;
2. deterministic discovery and validation from configured storage;
3. registration and collision handling for every supported provider class;
4. dispatch through existing MOS star-command, API, and C-function vocabulary
   where that vocabulary applies;
5. an explicit namespace for new project-owned services such as EDU;
6. Core MOS versus optional-provider boundaries;
7. residency, loading, unloading, replacement, dependency, nested-call,
   reentrancy, interrupt, callback, and teardown rules;
8. a defensible treatment of persistent/asynchronous services that cannot be
   naively swapped out of the single 32 KiB moslet/module area;
9. module-safe, module-compatible, unheadered, moslet, Z80-mode, and ADL-mode
   application behavior;
10. failure isolation and bounded recovery for missing, malformed, incompatible,
    oversized, conflicting, interrupted, or failed modules;
11. a defined lifetime for pointers returned through `mos_getfunction` and
    equivalent handles;
12. machine-readable manifests and deterministic validators/generators rather
    than prose-only registries;
13. at least one simple generic example provider and one bounded
    Extender-shaped provider or fake service; and
14. emulator regressions proving ordinary non-aware MOS programs continue to
    work and unsafe module requests fail without corrupting their memory.

Prefer the smallest generally useful Core MOS substrate over a private pile of
Extender hooks. Prefer an upstream-compatible extension when it satisfies EMOS.
When the upstream proposal cannot satisfy persistent transport, interrupt,
queue, or callback ownership, document the mismatch explicitly and keep the
local solution replaceable.

## Internet and repository research

Do not assume the two local précis exhaust the available art.

1. Recheck official MOS documentation, issues, pull requests, milestones,
   releases, nightlies, branches, and commit history at task start.
2. Search current forks and contributor repositories—not only issue prose—for
   loaders, relocation tables, registries, executable headers, dispatchers,
   overlays, ROM-extension systems, and abandoned experiments.
3. Follow linked issues for Unified I/O, joystick/keyboard APIs, streams,
   expressions, C functions, executables, moslets, and system variables.
4. Search analogous constrained systems where designs are transferable: BBC
   Micro sideways ROMs, RISC OS relocatable modules, MSX-DOS relocation,
   CP/M overlays and RSXs, and other Z80/eZ80 resident-extension systems.
5. Distinguish source code from discussion, maintainer direction from individual
   suggestion, current design from superseded design, and direct evidence from
   inference.
6. Preserve a dated, linked research digest so later agents can monitor small
   upstream changes without repeating an unbounded search.
7. Discord-only evidence is presently unavailable. If access becomes available,
   record author, channel, date, stable message link, and authority level; do
   not treat chat consensus as an accepted upstream specification without
   corroboration.

Use primary sources for technical claims. Search broadly, but do not let
research become an excuse to avoid producing the authorized implementation.

## Work protocol

1. Inspect both repositories and all local dirt before planning.
2. Write a detailed numbered implementation plan and explicit non-goals before
   changing source. Maintain it as work proceeds.
3. Build the unchanged selected baseline and run the applicable existing gates.
   If it does not pass, diagnose and preserve that fact rather than weakening a
   test.
4. Create structured decision and requirement inventories before coding the
   loader or ABI. Every provisional choice needs rationale, alternatives,
   compatibility effect, replacement path, and tests.
5. Implement in narrow vertical slices. Keep maintained MOS source changes,
   AgonDev build-harness changes, generated artifacts, and evidence visibly
   separate.
6. Add detailed comments beside deviations, workarounds, inherited defects, and
   unusual ownership machinery. Name provenance and removal conditions for
   upstream workarounds.
7. Test at the smallest useful level, then run `make firmware-check` and the
   relevant emulator gates. Add negative, malformed-input, memory-overlap,
   interruption, and compatibility tests, not just happy paths.
8. Use the emulator aggressively, but do not claim physical SD timing,
   electrical behavior, power-order safety, or hardware qualification from Fab.
9. Keep the existing human emulator-approval and commit rules in `AGENTS.md`.
   An unattended run may leave a complete reviewed diff and evidence without
   committing it when that gate applies.
10. At each coherent milestone, reconcile the live numbered plan, review the
    bounded diff and verification evidence, then commit and push the milestone
    in its owning repository. Use separate maintained-source and AgonDev-harness
    commits when both repositories change. These checkpoints are required
    fallback points during unattended work; do not defer all history until the
    final candidate. A milestone checkpoint does not override item 9: any
    emulator-coupled change remains uncommitted and unpushed until the Author
    completes the required launcher check and explicitly approves it. If a
    commit or push fails, preserve and record the exact repository state and
    failure, continue productive in-scope work, and retry at a later milestone;
    do not treat checkpoint transport failure alone as an implementation
    blocker or bypass a safety rejection.
11. End with a concise implementation report: contracts chosen, code changed,
    tests and emulator evidence, deviations from upstream, known limitations,
    open questions, and the exact next human decisions.

## Completion standard

An authorized implementation is ready for human review when:

1. clean-baseline provenance and repository roles are reproducible;
2. the first-version module/service ABI and lifecycle are documented and
   mechanically validated;
3. maintained EMOS source builds through AgonDev without hand-edited generated
   output;
4. existing MOS regression gates still pass or every intentional delta is
   isolated and justified;
5. module discovery, dispatch, compatibility declarations, collisions,
   malformed inputs, loading/unloading, failure recovery, and example providers
   have deterministic tests;
6. Fab boots the EMOS candidate and exercises the declared application-visible
   contract;
7. non-aware executables and ordinary stock MOS operations have bounded
   regression evidence;
8. unresolved Extender decisions remain unresolved rather than being silently
   frozen by implementation accidents; and
9. the report states precisely what is implemented, provisional, emulator-only,
   unqualified on hardware, or still absent.

Do not describe the result as the definitive upstream MOS Modules
implementation. It is the first fully realized **EMOS** modular-service
implementation, designed for upstream review and later convergence.
