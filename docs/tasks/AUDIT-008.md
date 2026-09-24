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

A08-E03 [ ] Validate admission/error paths, stock MOSlet ABI in emulator and
resident linked contracts. Measure ROM/RAM against 131056-byte baseline. Existing
historical provider tooling is retained for evidence, not current qualification.

A08-E04 [ ] Hardware acceptance after bench is released and deployment authorized.
No claim of physical keyboard, SD or UART validation from local checks.

No resident UART diagnostic extraction, new executable format, background loader,
plugin registry or physical utility migration is included. Existing listener is
already disk-resident; adding its `/emos` invocation does not itself save ROM.
The Author authorized unattended commits without waiting for visual review for
this run; human/hardware acceptance remains separate.
