# INTEG-001 — Implement the PORT-008 forward-only EMOS adapter

## State

- Status: Completed — implementation, Author emulator validation, and exact
  Extender handoff recorded
- Started: 2026-08-30 21:11 EDT
- Finished: 2026-08-30 21:57 EDT

## Intent

Supply the eZ80-owned half of `agon-extender` PORT-008's bounded forward-only
prototype. A fixed-purpose development EMOS build must route unchanged ordinary
VDU bytes through the preserved `light2-harness-r01` eight-bit parallel sender
to the retained P4 VDU parser. It must keep P4-to-eZ80 return traffic disabled,
start and recover in Legacy, and use the accepted Exclusive Extended mode
request as the only route-commit authority.

This task is prototype evidence, not the production transport contract. It
must not invent a VDU/EDU application envelope, parser, discovery protocol,
return packet, sysvar writer, or automatic activation path.

## Frozen inputs

1. `agon-extender` commit `c03656c` provides the compile-valid P4
   `ForwardParallelStream`, retained VDP v2.16.0 parser, browser display path,
   active bench constraint BC-001, and PORT-008 execution record.
2. `light2-harness-r01` maps eZ80 Port C bits 0--7 to data, Port D bit 5 to
   CLOCK, Port D bit 7 to active-low VALID, and Port D bit 4 to active-low
   READY from P4 GPIO20.
3. Official MOS `RST 10h` and `RST 18h` semantics send unframed raw binary VDU
   bytes. Official General Poll is byte sequence `23, 0, 0x80, n`; its response
   is deliberately unavailable in this forward-only tranche.
4. The accepted visible fixture is the exact 106-byte stream and independent
   RGB888 oracle already frozen by `agon-extender` PORT-003 Phase F.
5. EMOS Core owns ordinary VDU route commitment. The existing dispatcher
   snapshots `emosVduBackend`; Legacy and Dual retain onboard UART0, while this
   prototype may satisfy only Exclusive Extended.

## Work plan

1. **Build boundary.** Add one explicit AgonDev source profile for the
   fixed-purpose PORT-008 build. Keep the ordinary EMOS profile unchanged and
   fail closed when the prototype profile is not selected. Any generally useful
   source-profile plumbing belongs in `mos-agondev`, not in EMOS behavior.
2. **Physical sender.** Implement a bounded eZ80 sender using the r01 GPIO
   mapping and predecessor admission/VALID/CLOCK idiom. Save and restore the
   affected Port C/Port D registers; leave Legacy electrically unchanged;
   limit each physical record to the P4 receiver's 4096-byte maximum; preserve
   byte order; and return explicit READY-admission or completion failure.
3. **Mode adapter.** Bind the sender behind the resident EMOS adapter seam only
   in the fixed-purpose build. Exclusive Extended prepare must acquire the
   r01 pins and transmit an ordinary nonzero General Poll request so the
   retained P4 parser exits its stock startup wait. Commit ordinary VDU routing
   only after the record completes. Legacy recovery must release and restore
   the GPIO state. Exclusive Compatible and Dual remain unavailable through
   this adapter.
4. **VDU dispatch.** Route `RST 10h`, `RST 18h`, and C-runtime output through
   the same committed backend without changing their application-visible byte
   contracts. Prefer one physical record for a bounded `RST 18h` block and
   preserve delimiter-mode return semantics. A single-character call may use
   a one-byte record. Transport failures must not silently fall through to the
   onboard VDP.
5. **Cold-boot fixture.** Build a small ordinary ADL application containing
   the accepted 106 bytes and invoking the normal `RST.LIL 18h` surface. Define
   its exact root `/autoexec.txt` invocation, fixed startup delay/order, required
   files, terminal state, and non-keyboard browser evidence. The application
   must not access GPIO or a new Extender API.
6. **Deterministic checks.** Add source/profile tests for inactive-by-default
   selection, exact r01 pins, register preservation, record-size bounds,
   General Poll bytes, route eligibility, return exclusion, fixture bytes/hash,
   and generated build inputs. Inspect the compiled eZ80 sender where timing or
   port-write order depends on generated code.
7. **Build and emulator gate.** Run the generic `mos-agondev` source-profile,
   translation, compile, link, runtime, and regression checks. Emulator tests
   may prove Legacy and fail-closed non-hardware behavior only; they cannot
   prove GPIO electrical behavior. Follow the repository's mandatory Author
   emulator-validation gate before committing any emulator-coupled change.
8. **System handoff.** Return exact source commits, artifact inputs, fixture,
   limitations, and proposed physical procedure to `agon-extender` PORT-008.
   Stop for Author review before assigning deployable identities, staging media,
   flashing EMOS or P4 firmware, changing wiring, powering a connected harness,
   or enabling any reverse UART path.

## Required failure posture

1. A build without the PORT-008 profile retains the unavailable physical
   adapter and cannot enter either exclusive mode.
2. READY timeout, record completion timeout, invalid length, reentrant send,
   or failed prepare leaves the public mode in Legacy and restores the previous
   GPIO configuration.
3. This prototype cannot validate the discarded General Poll response and must
   say so in status/evidence. Sender admission and completion are physical-link
   evidence only, not EDP identity or protocol-response evidence.
4. No failure redirects bytes to onboard UART0 after Exclusive Extended has
   committed. Recovery requires an explicit Legacy request or bounded adapter
   rollback during failed preparation.

## Execution record

1. The fixed-purpose `port/port008-forward.mk` profile selects the physical
   adapter only through `EMOS_PORT008_FORWARD`. The ordinary profile does not
   define it and builds with the unavailable adapter selected after reset.
2. `src/serial.asm` owns the r01 Port C/Port D sender beside the semantic VDU
   dispatcher. It snapshots and restores all affected GPIO registers, preserves
   interrupt enable state, rejects empty/reentrant/over-4096-byte records,
   reports bounded READY admission/completion failure, and has no reverse-UART
   call.
3. `RST 10h`, C `putch`, and delimiter-mode `RST 18h` use one-byte records.
   Standard bounded `RST 18h` uses one physical record while preserving the raw
   VDU bytes and the stock postcondition of advanced HL and zero BC. Legacy
   retains the byte-at-a-time onboard UART0 path.
4. Exclusive Extended preparation acquires r01 and sends exact General Poll
   bytes `23, 0, 0x80, 1`; only then may EMOS commit backend 2. Exclusive
   Compatible and Dual remain unavailable through this fixed-purpose adapter.
5. The deterministic fixture contains the frozen 106-byte command at SHA-256
   `b5d2757ebf1bdaf0132b8a2a6683e749aa57aeb265ad23a25368451860f65595`.
   Its generated `P8VDU.BIN` is 118 bytes including a 12-byte ordinary
   `RST.LIL 18h` wrapper; its current full-artifact SHA-256 is
   `3befe47e271f0351222ce1748a40bea5b8650f99f55069e1183f73d19f6e754a`.
6. The linked sender verifier inspects assembled code for the READY admission
   and release loops, one Port C write followed by two Port D writes per byte,
   falling-edge order, the 4096-byte bound, and linked General Poll bytes.
7. The ordinary no-macro image built from the same prepared source is 113,805
   bytes at SHA-256
   `0122b6dd6a82b72b7b11364e0de7ce8c8c008b43f8087102f3f5b485afd9c7aa`.
   The PORT-008 profile image is 113,935 bytes at SHA-256
   `1675baf089ecd1420b59a546e5316519ec7b20e7000a3fe3f2c4493604984f9b`.
   These are build measurements, not deployable version identities.
8. The complete `mos-agondev` configured-input gate passed with the PORT-008
   profile and external Fab 1.2.3 environment: source preparation, translation,
   C compatibility, assembly, restricted runtime closure, link/image checks,
   headless boot, stock shell parity, VDP regressions, and target contract/FatFS
   persistence all passed. This is non-physical evidence only.
9. At 2026-08-30 21:54 EDT the Author ran the graphical Fab candidate and
   approved the emulator gate and commits. The displayed evidence showed the
   Platform VDP 2.16.0 and EMOS MOS 3.0.2 banners; discovery of all three
   provider fixtures; Legacy to fake Dual and back to Legacy with the
   `port008-forward` adapter restored; successful keyboard entry; root `dir`;
   `help echo`; `time`; `credits`; `mem`; `cd bin`; `/bin` directory listing;
   and return to `/`. The candidate remained responsive at the prompt.
10. Reusable profile, runtime-audit, named-worktree, and launcher support is
    frozen in `mos-agondev` commit `5079d4c`. The launcher correction followed
    a safe first attempt that built and verified the candidate but stopped
    before Fab opened because duplicate managed arguments were rejected.
11. The exact system handoff is frozen in `agon-extender` commit `da78d36`,
    under `docs/tasks/PORT-008.md`. It records all three source identities,
    fixture and build measurements, approved emulator observations, bounded
    non-physical claims, and the separately authorized physical gate.
12. The subsequent physical-candidate preparation assigns source identity
    `agon-emos-v0.1.0` and lifecycle state `candidate`. A UTC build identity is
    supplied only by a controlled build; all other builds report
    `UNVERSIONED-DO-NOT-DEPLOY`. On 2026-08-31 the Author visually confirmed
    that the graphical Fab candidate reported that exact fail-closed identity,
    completed Legacy--Dual--Legacy transitions and service calls, and remained
    responsive at the MOS prompt, then explicitly approved the candidate for
    commit.

## Gotchas and remedies

1. Initial assembly exposed short-branch range failures after adding the
   sender. Required branches were made explicit or the unchanged stock helper
   was kept adjacent to its original callers; the official UART putch blocks
   still pass byte-for-byte comparison against their upstream source.
2. The first complete runtime audit scanned `mos-agondev`'s default prepared
   tree rather than the configured r3 source and therefore could not see the
   two product-owned assembly exports. Generic build plumbing now passes the
   configured maintained-source tree explicitly as `ASSEMBLY_SOURCE`; the
   audit remains fail-closed and product-neutral.
3. The installed Fab checkout lives outside `mos-agondev`'s default path. Its
   ignored local profile was safely refreshed against the existing checkout;
   no emulator executable or firmware was downloaded or replaced.
4. The first Author launch attempt rebuilt and verified the correct candidate
   but stopped before opening Fab because `mos-agondev`'s Make target duplicated
   launcher-owned `--renderer`, firmware, MOS, SD-card, and verbosity options.
   The generated launcher correctly rejected the conflict. The reusable target
   now invokes that launcher without overriding its managed arguments, with a
   regression test preserving this ownership boundary.
5. The outer `agon-emos make qualify` wrapper did not propagate its selected
   prepared MOS worktree to the final repository-local Python test invocation.
   Every preceding `mos-agondev` qualification gate passed, and all 51 local
   tests passed when invoked directly with the same worktree through
   `MOS_AGONDEV_WORKTREE`. This is wrapper-variable plumbing, not a firmware or
   emulator failure; the exact direct invocation remains the qualification
   evidence for this candidate.

## Known limits at review boundary

1. A single ordinary `RST 18h` block above 4096 bytes is rejected rather than
   split into multiple physical records. The accepted fixture is 106 bytes;
   compatibility treatment for larger blocks is follow-on work, not evidence
   from this tranche.
2. The General Poll response is discarded. Successful prepare proves only
   READY admission and physical record completion, not EDP identity or a valid
   response packet.
3. No reverse transport, sysvar update path, EDU envelope, discovery protocol,
   or automatic activation was added.
4. Emulator qualification cannot exercise the r01 GPIO electrical path. The
   repository requires the Author to validate the candidate with the graphical
   launcher before any emulator-coupled source is committed.

## Completion boundary

INTEG-001 completes when the fixed-purpose EMOS and ordinary fixture build,
their deterministic checks pass, the Author accepts any required emulator run,
and the exact handoff is recorded in PORT-008. Physical deployment and visible
browser evidence belong to PORT-008 and require separately committed identities,
procedure review, and bench authorization.
