# AUDIT-008 — Resident simplification and foreground EMOSlets

Started: 2026-09-24 UTC. Author-approved first tranche; compile/link and isolated
emulator validation only. Bench and physical SD access prohibited this run.
The cross-component contract is in agon-extender docs/tasks/AUDIT-008.md;
its source research pins official MOS v3.0.2 and AgonDev load/run conventions.

A08-E01 [x] Remove cancelled external provider registry, discovery, CRC/load/swap
and generic MOS command fallback. Keep resident gateway ABI, services, lifecycle,
UART, keyboard and mode coordinator. DISCOVER/CLEAR are reserved unavailable.

A08-E02 [x] Add built-ins-first `/emos/<name>.bin` MOSlet dispatch, idle/Core only.
Accept 1–24 ASCII letters/digits/underscore/hyphen, lowercase the leaf; no path or
variable expansion. Use stock mos_LOAD and mos_runBin at B0000 with a 32 KiB size
ceiling; do not alter Moslet$Path/Run$Path or application RAM. Preserve stock
return codes and argument forwarding. Utilities remain trusted executables:
a standard MOS header cannot prove their link address or runtime memory use.

A08-E03 [x] Validate admission/error paths, stock MOSlet ABI in emulator and
resident linked contracts. Measure ROM/RAM against 131056-byte baseline. Existing
historical provider tooling is retained for evidence, not current qualification.

A08-E04 [ ] Hardware acceptance after bench is released and deployment authorized.
No claim of physical keyboard, SD or UART validation from local checks.

No resident UART diagnostic extraction, new executable format, background loader,
plugin registry or physical utility migration is included. Existing listener is
already disk-resident; adding its `/emos` invocation does not itself save ROM.
The Author authorized unattended commits without waiting for visual review for
this run; human/hardware acceptance remains separate.

## Local result — 2026-09-24

Final ordinary draft build `agon-emos-v0.1.19-b2026-09-24-02-02-56Z`, product
source `49cbd36`, occupies 124774 ROM bytes with 6298 free. It recovers 6282 ROM
and 3155 static RAM bytes. All selected-profile firmware/link guards and all 91
host tests pass. The official Fab 1.2.5 CLI harness passes autoexec plus 28
commands in 39.3 host seconds, including a 4096-byte application sentinel,
re-entry, return code, negative executable/name tests and nested launch denial.
Existing sdserve launches and returns unavailable without a peer; no live
transfer claim. All durable measurements/transcripts are in the Extender
AUDIT-008 implementation evidence. A08-E04 remains pending: bench untouched.

The routine contract is [foreground EMOS utilities](../emos-utilities.md).

## Authorized physical follow-through — 2026-09-24

Author released bench and requested deployment. v0.1.19 is installed with full
128 KiB readback equality. ExCom/Legacy, injected keyboard, mixed-case /emos
listener dispatch, fast/normal transfers and 4096-byte application sentinel pass.
Listener moved to /emos/sdserve.bin; startup and P4/VDP unchanged. A08-E04 stays
open for broader acceptance; this is no longer an uninstalled candidate. Exact
evidence is in agon-extender docs/tasks/AUDIT-008/HARDWARE.md.
