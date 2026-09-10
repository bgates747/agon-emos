# INTEG-011 — Preserve displays during application-requested route switches

Status: Implementation and visual review accepted; candidate hardware testing through Extender QUAL-003 is authorized.
The Author requested application switching and authorized implementing an
interface where needed. Existing mos_oscli already executes EMOS commands.
The Author accepted keeping both images visible with this explicit option.

1. Extend case-insensitive `EMOS EXCOM` / `EMOS LEGACY` with optional
   `--keep-display`, using the existing coordinator and route publication.
   Default console switching retains its fresh-display behavior. Reject unknown
   options before changing state. Reset per-request options even after failure.
2. Keep mainboard and P4 scenes/resources in their respective processors.
   Do not clear, reset their modes or print mode notices for this option.
   A distinct P4 prepare-keep opcode must be acknowledged by the paired P4;
   old firmware must fail entry rather than silently reset its display.
   Post-commit mode/cursor/poll synchronization still governs EDP publication.
3. Preserve USB keyboard selection and mainboard clock. Do not introduce a
   second application transport path or a new public RST ABI. The fixture
   wrapper checks MOS return status and obtains current display information
   before relying on mode-dependent sysvars after mainboard return.
4. Test real adapter behavior, command parsing, kept/fresh initialization,
   rejection by old peers, and existing mode transaction failures. Build only
   through the guarded wrapper; measure remaining 128-KiB image headroom.
   Perform human emulator validation before any commit or physical deployment.

Source references: official agon-docs mos/API.md 0x10; maintained src/emos.c,
src/emos_console.c and paired src/emos_console_wire.h. No official reference
checkout is changed. This is explicit application-controlled switching at
complete VDU boundaries, not transparent arbitrary-application migration.
Standing preapproval supplies draft EMOS v0.1.12. Product specification and
paired fixture are owned by agon-extender QUAL-003.


Draft validation: exact coordinator/command-branch and real adapter tests pass.
The live-application guard now admits the explicit retaining Legacy/ExCom
request while preserving busy and other-mode rejection. Guarded final firmware
is 130215 bytes (857 bytes below 128 KiB). Extender QUAL-003's complete native
review exercised 24 paired Shapes pages, 123 paired bitmap stages, early Escape
and truncated-asset recovery, returning to ExCom MOS. Human review is pending.


On 2026-09-09 the Author accepted the paired images without saving a screenshot
and requested flashing/hardware testing. EMOS v0.1.12 advances to candidate;
freeze this reviewed implementation before the guarded candidate build.
Physical application switching remains pending in Extender QUAL-003.
