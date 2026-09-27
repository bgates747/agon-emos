# INTEG-012 — Private graphics benchmark completion reception

Status: Private callback reception is implemented and used by the later paired
[game timing package](https://github.com/bgates747/agon-extender/blob/main/docs/testing/game-timing.md)
and retained QUAL-003 captures. Those bounded physical results supersede the
original hardware-pending checkpoint below. Diagnostic instrumentation and known
mainboard failures remain distinct from a production generalized-callback ABI;
no such API is qualified by these results.

## Current applicability — 2026-09-26

This remains a private diagnostic use of the existing single keyboard callback,
not an additional callback slot, queued event API or production generalized
callback contract. The callback runs with normal ISR restrictions and receives
a pointer to transient protocol storage: validate its diagnostic discriminator,
copy needed data into bounded application storage and return promptly. Ordinary
keyboard callbacks still share that registered vector. Source token/metric
matching remains the application's responsibility, not proof supplied by EMOS.

Current code checks the diagnostic signature/discriminator and selected backend;
UART1 additionally requires a committed console lease. These checks preserve
routing ownership but do not qualify every malformed/stale sequence or workload.
The [timing guide](../../../agon-extender/docs/testing/game-timing.md) separates
submission, worker completion and physical scanout. A callback is not evidence
of a displayed frame, and instrumentation may alter scheduling. Follow the
[capture-control protocol](../../../agon-extender/docs/qualification/capture-failure-protocol.md)
when investigating capture-associated failures. Its checks and the retained
runner's reuse gate remain separate from this reception implementation.

The hardware-pending paragraph below is the original emulator checkpoint;
later results linked above supersede it only within their measured scope.
No public callback ABI or fresh test acceptance is introduced by this review.

## Original implementation contract

1. Preserve ordinary VDU routing and keyboard handling. UART0 diagnostic effects
   are accepted only in Legacy; UART1 only with a committed ExCom lease.
2. Receive new experimental response 8C, exactly 16 bytes: `QTG`, discriminator A1 (version 1; invalid keyboard down byte),
   token16, metric8, source8, value32, count32 (little endian). 8A/8B remain
   stock echo/echo-end and unsupported by EMOS, never repurposed.
3. Validate size/magic/version/source before invoking the existing registered
   user callback with DEU pointing at the private payload. Do not update keyboard
   sysvars, keycount or held-key map for diagnostic events. The app validates
   token/metric, copies to bounded RAM and clears its callback on all exits.
   This is a private experimental extension to mos_setkbvector dispatch, not
   the production generalized-callback ABI. Ordinary keyboard callbacks remain.
4. Use the current assembly dispatch and bounded C reply bridge; never bypass
   EMOS from the app. Verify the 128 KiB image limit, ROM/ABI/UART checks,
   selected-source dispatch, malformed/late/repeated packet rejection and normal
   keyboard behavior in the emulator. Leave emulator-coupled changes uncommitted
   until Author review.
5. Exact experiment contract belongs to agon-extender QUAL-003. No processor
   performance repair, parallel transport or public callback design here.

Research: official MOS API 0x1D preserves the single ISR callback; stock VDP
v2.16.0 packet IDs 0A/0B are echo/echo-end. Experimental 0C is separate.
Official references remain clean at MOS v3.0.2 and VDP v2.16.0.

## Accepted emulator review — 2026-09-11

The Author supplied the completed paired graphics review screen (256 intervals,
two known stock probe differences, final MOS prompt) and authorized deployment.
Full firmware/ROM/ABI/UART checks passed; the private replies leave keyboard
state unchanged. Promote v0.1.13 to candidate and freeze these source inputs
before the qualified build. Physical evidence remains pending in QUAL-003.
