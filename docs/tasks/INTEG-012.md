# INTEG-012 — Private graphics benchmark completion reception

Status: emulator reviewed; hardware candidate preparation. Author approved QUAL-003 execution and standing
versioning authority. EMOS v0.1.13 is a diagnostic development increment.

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
