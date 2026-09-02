# INTEG-002 — Implement the production forward-parallel data plane

## State

- Status: In progress — production engine, semantic dispatcher bridge, fixed
  qualification composition, and UART1/Port C ownership integrated, validated
  in a pre-freeze dirty-source snapshot, and source-frozen; target proof,
  build/object provenance, artifact identity, and physical gates remain open
- Started: 2026-09-01
- Finished: --

## Intent

Replace the fixed-purpose INTEG-001 sender with maintained production EMOS
objects that can run unchanged on the present r01 forward wiring and the
intended Extender circuit from an already-authorized parallel epoch inward.
Keep EMOS as the sole ordinary-VDU route and mode-commit authority. Preserve
the official raw VDU byte stream and upstream-shaped RST 10, RST 18, and
C-runtime entry points.

The cross-component authority is `agon-extender` PORT-008-D002 and its
task-local current-circuit production-equivalence audit. This repository owns
only the EMOS/eZ80 implementation.

## Immediate work

1. [x] Add one production parallel-epoch owner as the sole EMOS actor that
   acquires and releases eZ80 `PC0..PC7` and `PD4/PD5/PD7` mux, direction, and
   idle state. It receives externally established peer readiness; it does not
   define activation or commit a formal mode.
2. [x] Add one production record engine below the semantic VDU dispatcher. It
   accepts only a live epoch lease, sends raw bytes in order, bounds READY
   admission and release, restores idle CLOCK/VALID and busy state on every
   exit, and exposes an internal lifecycle result.
3. [x] Split a nonzero bounded RST 18 stream into physical records at or below
   the receiver limit without a record header or command envelope. Permit VDU
   commands to cross record and RST-call boundaries. Preserve zero BC, advanced
   HL, and the current de-facto successful A=0 result unless a later explicit
   compatibility decision changes it.
4. [x] Keep the per-byte critical section no larger than required by the
   qualified GPIO cadence. Do not disable interrupts across READY waits or a
   complete maximum-size record. Record the exact interrupt and UART-ingress
   coexistence contract beside the hot loop.
5. [x] Add a qualification-only fixed-backend profile whose sole substitution
   is a procedure-controlled assertion that the operator/top-level has already
   prepared the peer epoch. It calls the production epoch and record objects,
   emits no activation or precommit General Poll, is excluded from
   normal/release profiles, and creates no supported application bypass.
6. [x] Add deterministic source/model tests for all byte values, zero/one/
   boundary/over-boundary lengths, multi-record chunking, READY timeout and
   release timeout, reentry, cleanup, register postconditions, interrupt-state
   preservation, coordinator recovery, public/private error mapping, and
   failure propagation. Add registered linked-image checks for object
   inclusion, helper instruction shapes, semantic route and coordinator call
   edges, common-lease ownership, UART1 exclusion, and predecessor absence.
7. [ ] Extend target-derived proof to the remaining C storage spans, record-
   loop bounds, and indirect GPIO cadence/port-write ordering. Structural call
   fingerprints establish reachability, not execution of a particular branch.
8. [ ] Freeze target compile commands and production-object digests for the
   qualification composition. Hand those records to PORT-008 for comparison
   with the eventual release composition; native host tests do not establish
   eZ80 target-object identity.

## Reuse and replacement boundary

1. Reuse the existing reset-vector entry points, semantic VDU dispatcher,
   coordinator-owned backend snapshot, and canonical MOS packet parser.
2. Do not promote `PORT008_send`, `_emos_port008_prepare`, the precommit
   General Poll, the r01 register-snapshot policy, spin-count timing, whole-
   record interrupt exclusion, 4096-byte call rejection, or the exact
   `port/port008-forward.mk` product behavior.
3. Retain INTEG-001 and its failed/qualified evidence as predecessor history.
   Production code must not include an r01 conditional, alternate VDU grammar,
   fake success, silent onboard fallback after commit, or direct application
   transport bypass.

## Dependencies and gates

1. Official `agon-docs` defines the raw VDU stream and RST 10/RST 18 ABI.
   Official MOS source supplies the maintained control-flow baseline. The
   recorded bounded-RST-18 documentation/source return-value divergence is
   inherited and deliberately not corrected in this task.
2. PORT-008 owns P4 endpoint readiness, cross-processor epoch authorization,
   qualification composition, target-object comparison, and any physical run.
3. Production activation, General Poll confirmation, UART1 response parsing,
   MOS sysvar updates, shutdown/reset recovery, and whole-mode qualification
   remain out of scope until their cross-component decisions are accepted.
4. Do not edit generated `mos-agondev` source or outputs. Build only through
   repository wrappers or a reviewed EMOS source profile with every registered
   link check enabled.
5. Any emulator-coupled result requires Author validation before commit or
   push. This task did not itself authorize a commit; the Author separately
   authorized the source freeze after the recorded validation. Neither that
   authorization nor this task authorizes a deployment identity, media
   mutation, flash, wiring, reset, power, or physical operation.

## 2026-09-01 implementation record

1. New maintained units now separate a hardware-independent physical-record
   engine, the eZ80 local pin/epoch binding, and seven IFF-preserving assembly
   helpers. The helpers cover masked Port D updates, pin configuration, writer
   admission, atomic entry start, atomic Port C reservation, a four-byte clock
   snapshot, and epoch publication. The engine chunks at 4096 bytes, emits no
   wire header, bounds both elapsed READY time and a stalled-clock liveness
   fuse, leaves interrupts enabled outside tiny register/clock operations, and
   preserves the admitted writer when a nested writer is rejected.
2. The eZ80 binding uses the upstream MOS four-byte centisecond clock. Official
   `interrupts.asm` advances that clock in the hardware VBLANK ISR, independently
   of UART response parsing. The assembly snapshot helper masks interrupts only
   while copying those four bytes and restores the caller's IFF2 state. The
   `0xFFFFFF` poll limit counts only consecutive polls during which that clock
   does not advance; it is an uncalibrated fail-closed liveness fuse, not the
   elapsed-time authority or a promoted prototype spin-count timeout.
3. The binding preserves unrelated Port D bits with masked read/modify/write,
   drives the READY output latch high while PD4 remains an input, and uses
   per-call atomic lock results. Separate IFF-preserving begin/commit helpers
   make first initialization and entry publication interrupt-safe, preserve a
   live lease on redundant entry, and force an in-progress entry to abort if a
   writer, leave, or another enter contends before commit. A rejected nested
   writer remains a sticky deferred BUSY lifecycle fault. The admitted writer
   completes and returns its controls to idle; subsequent routing remains
   closed until a successful leave/re-enter lifecycle reset. Epoch generations
   are 24-bit and fail closed instead of wrapping after `0xFFFFFF` acquisitions.
   A caller that interrupts the one-time boot initialization receives BUSY but
   does not create a sticky lifecycle fault because no epoch exists yet; the
   boot/coordinator owner may retry after initialization publishes complete.
4. Official `agon-docs` defines the bounded RST 18 length in `BC`, while APIs
   that accept a 24-bit count are explicitly documented as `BC(U)`. Pinned
   official `vectors16.asm` likewise tests and decrements `BC` and tests only B
   and C for completion. The production record API therefore accepts `UINT16`.
   The integrated dispatcher bridge keeps delimiter mode (`BC=0`) on the byte
   path, passes a bounded 16-bit count to one stream call, advances HL by that
   count, zeros BC, preserves E, and retains the de-facto successful A=0 result.
5. Deterministic host checks execute the exact production engine and binding.
   They cover all byte values, zero/one/4095/4096/4097/65535 lengths, exact
   cadence and flattening, elapsed and stalled-clock timeouts, clock wrap,
   register release/entry postconditions, UART1 admission rejection, failed
   peer retry, stale and redundant leases, generation exhaustion, interrupting
   first initialization, pre-peer and peer-callback enter/write/leave
   contention, nested writer non-interleaving, sticky deferred failure, and
   lifecycle recovery. Linked-image inspection requires one defined global
   text symbol for every production entry point, nonempty engine instruction
   extents, the exact IFF2 save/conditional-restore sequence and bounded
   instruction count of each helper, exact Port D masks, and the four-byte
   atomic clock copy. The integrated checks additionally require the ordinary
   entry-to-dispatcher-to-common-route call graph, UART1 guard calls, and
   predecessor-symbol absence. They do not authenticate source/object or
   toolchain origin and do not yet prove every C storage span, the record-loop
   bound, or indirect GPIO cadence; Immediate Work items 7 and 8 remain open
   for final target proof and provenance/qualification.
6. The non-release fixed profile is now a complete ordinary-VDU qualification
   composition. Its procedure requires the operator/top-level to prepare the
   peer before making the explicit Exclusive Extended mode request; EMOS has no
   independent external-attestation channel and does not detect peer identity.
   The fixed adapter exposes only common-route enter/ready/leave calls and no
   direct write API. RST 10, bounded and delimited RST 18, and C-runtime output
   reach the production engine only through the EMOS semantic dispatcher. Its
   build gate runs the linked RST/API, semantic VDU route, production-object,
   fixed-lifecycle, non-release-marker, and predecessor-absence checks.
7. The first isolated fixed-profile build exposed a repository-wrapper defect:
   the EMOS `Makefile` accepted `MOS_WORKTREE` but did not pass it to the
   generic `mos-agondev` firmware targets. A caller could therefore select one
   prepared source tree while the wrapper built the generic port's default
   tree. All EMOS build and qualification targets now forward the selected
   worktree explicitly, and a repository-local regression checks that
   forwarding. This was a local port/build-orchestration defect, not upstream
   MOS behavior.
8. The first dispatcher reconciliation made the UART1 exclusion conditional on
   a source-profile macro. That left an EMOS ZDS build able to compile the same
   Port C owners without the guard and was therefore a local profile-parity
   defect. The corrected EMOS source makes the guard unconditional, registers
   the parallel owner in every maintained EMOS build surface, and rejects any
   profile-optional guard in source and linked checks. `open_UART1()` acquires
   the common Port C transition lock before its first flag, mux, or UART
   mutation, publishes UART1 active before releasing the lock, and returns the
   existing generic UART failure when a parallel entry, writer, or live epoch
   owns the transition. This correction is project integration behavior, not
   an upstream MOS defect.

### Superseded pre-reconciliation integration boundary

The following blocker record is retained as the exact boundary before the v8
dispatcher and UART1 reconciliation. It is historical evidence and does not
describe the current source closure.

`src/serial.asm` already contained the upstream-shaped RST 10/RST 18 semantic
dispatcher and was outside this tranche's then-authorized edit scope. This
work did not edit, restore, stage, or otherwise reconcile that file.
Consequently the new production objects could be compiled and linked, but
ordinary RST/C-runtime traffic did not yet call them, and the fixed profile was
not yet a runnable dispatcher composition. The later reviewed v8
reconciliation replaced the prototype backend calls with the new lease-checked
entry points while preserving the ABI postconditions above.
At that point, the data plane, dispatcher, fixed composition, and their
validation remained open; no semantic-integration, fixed-composition, object-
equivalence, or qualification claim was made.

The clean-overlay image also still linked the complete predecessor PORT-008
adapter from the repository version of `src/serial.asm`, including its General
Poll, 4096-byte rejection, register snapshots, spin waits, and whole-record DI
path. The profile did not select that adapter, but symbol inclusion meant this
was not a production-only data-plane closure. The later dispatcher
reconciliation removed the predecessor path rather than merely leaving it
unreachable.

There was a second ownership gate outside these new files. Stock
`open_UART1()` cleared `serialFlags` bit 4 before changing the Port C mux and
set it only afterward; the existing MOS API could therefore race the epoch
admission guard or remux PC0/PC1 during a live parallel epoch. The later v8
integration serialized UART1 open against the whole owned epoch and closed
that software gate.

### Pre-reconciliation validation history

The following measurements bind the earlier partial composition only. They are
retained for traceability and must not be cited as measurements of the v8
integrated dispatcher composition.

1. Eleven focused clean-scope EMOS host/structural tests passed, and the complete
   EMOS repository suite passed 62 tests. The focused checks executed the exact
   production C engine and eZ80 binding against deterministic wire, register,
   time, and interrupt-contention models; the three then-excluded prototype/
   dispatcher files are not inputs to these historical claims.
2. The generic AgonDev port suite passed 108 tests after adding the product-
   neutral assembly profile surface. Three focused source-profile tests and
   16 focused runtime-tool tests also passed. The negative cases rejected absolute and
   parent-traversing paths, paths outside `src`/`src_startup`, non-assembly
   sources, duplicate source/object declarations, missing sources or linked
   objects, mismatched source/object pairs, source/object symlink escapes,
   undeclared provider permission, and declarations hidden by Make's sorted
   final suffix. Provider authority is derived from the linked target objects,
   not lexical XDEF text. The dated `mos-agondev` development log owns that
   completed generic change.
3. Both the normal profile and non-release fixed partial scaffold compiled and
   linked from an isolated source overlay that retained the repository versions
   of all three then-excluded files. The prepared source reports
   `0e24b06abdb322fdb4e681a21242ccfebfc8ea65+tracked-dirty`; the SHA-256 of
   its `.mos-agondev-worktree.json` metadata is
   `a7334e241d916d04b02826b10e58f567e2182e2ab55e382a7b80f9fcf3029a2c`.
   Each wrapper independently passes the 16 runtime-tool tests, runtime
   closure, linker layout, target IFF/masking inspection, UART divisor checks,
   and firmware image validation. The final normal image is 116730 bytes with
   SHA-256
   `ff5ab90dcb475e25b0961b9def8cb2da359359d5f8e9b06a49c15fbac23cdc68`;
   its ELF SHA-256 is
   `eb939188f926f1fb9a8ea65d018e402455d0fdafb07b2add9d58eee09e480782`.
   The fixed partial scaffold is 116819 bytes with SHA-256
   `e6c6e43b007e4190a8c8ddc92fb9ec90ae1b13d04437dc60ef03e1ab3fbb96dc`;
   its ELF SHA-256 is
   `45eef6ae524156594d95c6d51fb46bcfc2985975bf1b03ab811a9f846a3cf38a`.
   The fixed build additionally passes the existing EMOS API/RST and bounded
   semantic VDU-dispatch checks. The normal profile was built last and remains
   the final local output.
4. The two builds produced matching local production-object digests:
   `emos_parallel.o` is
   `7d7caf7e7b628a78fc1610d66c69ec6735dee37f62333ad2f0b834a57a122aca`,
   `emos_parallel_engine.o` is
   `48b19e48ace8e75061cc51e99dd074e2b0f80f9fcf9b93cb5f2a29d723d0ebfc`,
   and `emos_parallel_io.o` is
   `5599b68dc1bf316af848034763a1677eb0a13e158412e0e5e91e8b5964b579f6`.
   Matching loose-object bytes do not establish that either final linker
   consumed those exact objects, do not authenticate the toolchain or source
   origin, and cannot satisfy current Immediate Work item 8.
5. The overlay reports `tracked-dirty` because this tranche was expressly not
   committed. That state means `HEAD` alone is an incomplete source identity;
   it does not by itself invalidate the result. These implementation checks are
   not frozen target-object provenance because they lack an exact retained
   diff/input closure, frozen compile commands and toolchain identity, and
   final-link lineage. PORT-008 records the local digests only as diagnostics.
   Current Immediate Work item 8 remains open until those missing authorities
   are supplied for the actual ordinary-VDU composition. No emulator,
   deployment, activation, UART return, or physical operation was run.

### v8 dispatcher and UART1 reconciliation

1. `src/serial.asm` now contains only one semantic route decision for each
   ordinary entry. C `putch`, RST 10, and delimited RST 18 call the byte
   dispatcher; bounded RST 18 calls the block dispatcher. Each dispatcher
   snapshots `emosVduBackend` once. Backend 0 uses only onboard UART0, backend 2
   uses only the common production route bridge, and every unsupported value
   fails closed without UART fallback.
2. The common route bridge alone retains the private live lease. Its byte and
   bounded-stream entries call the production record engine; the fixed adapter
   can only enter, inspect, and leave that route. The fixed composition binds
   those lifecycle calls to the existing explicit Exclusive Extended mode
   request after the operator/top-level has prepared the peer. It emits no
   activation, General Poll, response, or supported application transport API.
3. The maintained sender, mode adapter, General Poll, spin waits, 4096-byte
   whole-call rejection, register snapshots, and whole-record interrupt mask
   from INTEG-001 are absent from both maintained source and linked closures.
   `port/port008-forward.mk` is a fail-fast tombstone; only the ordinary payload
   fixture remains executable as predecessor evidence.
4. `open_UART1()` now acquires the same atomic Port C transition lock used by
   epoch entry and writers before changing `serialFlags`, Port C mux state, or
   UART1 registers. UART1 publishes its active flag before releasing that lock.
   A parallel owner therefore rejects UART1 activation, and a UART1 activation
   already holding or publishing ownership rejects parallel entry. This guard
   is unconditional in EMOS and is no longer selected by a source-profile
   macro.

### Historical v8 integrated validation

This subsection was the then-current reconciliation result. It is superseded
by the v10 result below and must not be cited as the current artifact record.

1. The bounded source snapshot contains 145 files and reports HEAD
   `0e24b06abdb322fdb4e681a21242ccfebfc8ea65+tracked-dirty`. The dirty suffix is
   material only because `HEAD` does not identify the included changes. These
   are implementation results, not a frozen target-provenance record; the
   exact input/diff closure, compile commands, toolchain, and final-link lineage
   were not frozen.
2. All 63 EMOS repository tests pass. The normal and non-release fixed profiles
   both compile and link, and their registered checks accept the ordinary
   dispatcher call graph, upstream-shaped ABI postconditions, production route
   entries, UART1 exclusion, fixed lifecycle binding, non-release markers, and
   predecessor-symbol absence. These checks do not authenticate the compiler,
   source origin, or eventual release composition.
3. The normal profile produces `MOS.bin` at 116536 bytes with SHA-256
   `b4c54b50882628a54242252b9ba962ed01198ccef7f0f374e9c58474fee9dd1f`.
   Its ELF SHA-256 is
   `c7a04c162faca36296438b9c0cd5d12a1a47c9a77bf725c25ec9a27632f046c8`,
   and its map SHA-256 is
   `776bc89ab2a48419349280b65e711e6dbadab84532563c9594ed557dd07b936a`.
4. The fixed qualification profile produces `MOS.bin` at 116836 bytes with
   SHA-256
   `54e6671b0e6f4276921e87a9fba4af2be3b66256621de092c9adc45a4302c1f9`.
   Its ELF SHA-256 is
   `5e0e930a9dfac10c19d21a015f0e4cf1d25f3c4c0e80e86fc82413f39956cba6`,
   and its map SHA-256 is
   `b59886eeb6a73cc822573e734d65d070f4f17086699e139bb42f398976ff2028`.
5. All five production objects common to the two profiles are byte-for-byte and
   SHA-256 identical. This is strong local profile-parity evidence, but loose
   object equality and linked artifact hashes do not freeze the exact changed
   source inputs, compile commands, toolchain, or final-link lineage and
   therefore do not satisfy Immediate Work item 8.
6. No emulator, deployment, peer activation, General Poll confirmation, UART
   response, flash, media mutation, physical operation, artifact identity,
   commit, or push was performed for v8. The remaining target proof and frozen
   target-object provenance are now Immediate Work items 7 and 8; every
   physical or cross-processor gate also remains open.

### Current v10 software-only validation

1. The isolated prepared-source snapshot contains 146 files and reports
   `0e24b06abdb322fdb4e681a21242ccfebfc8ea65+tracked-dirty`. All 64 EMOS tests
   pass, including exact extracted production coordinator transactions,
   recovery-failure state retention, successful adapter switching, every
   public result in `0..36`, private-result mapping, the production engine and
   binding, UART1 reservation, semantic dispatch, and predecessor retirement.
   The generic `mos-agondev` suite independently passes 108 tests.
2. The maintained normal profile builds a 116,520-byte `MOS.bin` at SHA-256
   `a4c2d3f87286dd32e7b2e3a7b30786ae4156208e2440ba096c733f76256d09a4`.
   Its ELF is
   `8315db0fd2f519a16d35d4b668d74ed9b6d52e65cee0c1ca947acf9b64f88373`
   and its map is
   `75a5a45de6099c1e12596752000defde76e5b46efb30705988792f5218179357`.
   The image contains the deliberate `UNVERSIONED-DO-NOT-DEPLOY` marker.
3. The non-release fixed profile builds a 116,966-byte `MOS.bin` at SHA-256
   `4f0c0db7d419c6a0f17fe9d07dd3bd2107fb110371fa0a44b444c1e3417d2e66`.
   Its ELF is
   `7186c10373f6f642eedc0949fb443cd5dd7b6eedeed43a44b86f877c78bb382d`
   and its map is
   `539375306a4f830ec300131e033b556df519fd0cee9819bff63364f4ffbcc341`.
4. The five common production objects are byte-identical across profiles:
   `emos_parallel.o` is
   `0001fd670e4782d2b051dad17bf8fd5fc0ea72bdba91deb100de702b3e7fc36b`,
   `emos_parallel_engine.o` is
   `48b19e48ace8e75061cc51e99dd074e2b0f80f9fcf9b93cb5f2a29d723d0ebfc`,
   `uart.o` is
   `c727d3dc77636697730e63ad64202a60ff4b64a346706376d56c9b0f1d38d2b5`,
   `emos_parallel_io.o` is
   `49d1bbd7471f48b44baf7012b02c95d15134d7f556f0a30c41f0c0903f5e878e`,
   and `serial.o` is
   `e0f7ddafdd52ab7ef3c3a7003343f76197157412f4235955467247f526344cfa`.
   `emos.o` differs as expected because only the fixed composition includes
   the fixed adapter selection and coordinator binding.
5. The registered linked checks bind the byte and bounded-block bridge
   instruction shapes, RST 18 argument and epilogue contract, one shared route
   lease, semantic route targets, unconditional UART1 guard, and fixed-profile
   coordinator call reachability. Call-count fingerprints do not prove that a
   target executes a named recovery branch. The extracted exact-source tests
   provide the corresponding coordinator path evidence on the host.
6. These are current implementation checks, not frozen provenance, target-
   execution, activation, UART-return, electrical, or physical qualification.
   The `tracked-dirty` suffix means `HEAD` alone is incomplete identity; Work 8
   remains open because the exact changed-input closure, compile commands,
   toolchain, and final-link lineage are not frozen. Immediate Work item 7 and
   all cross-processor/physical gates also remain open.

### Source-freeze disposition

After reviewing the v10 and stale-state records, the Author separately
authorized a repository source-freeze commit. That commit preserves the
implemented EMOS boundary and its tests; it does not retroactively make the
v10 prepared artifacts clean-source builds, assign a firmware identity, prove
final-link object provenance, or close target, activation, return, deployment,
electrical, or physical gates. A fresh identified build remains required for
qualification.

## Completion criteria

1. [x] Production epoch and record objects replace, rather than wrap, the exact
   prototype data-plane implementation.
2. [x] Deterministic exact-source tests and registered structural linked-image
   checks pass, including multi-record RST 18 behavior and EMOS coordinator/
   record-engine failure cleanup.
3. [x] The fixed-backend profile is visibly non-release and has no activation,
   response-delivery, or supported-bypass claim.
4. [ ] Remaining target-derived loop/storage/cadence proof is complete.
5. [ ] PORT-008 receives frozen target-object/provenance records and an explicit
   list of claims that still require the intended circuit or Author interaction.
