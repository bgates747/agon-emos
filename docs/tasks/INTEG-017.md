# INTEG-017 — External text diagnostics and resident scaffold removal

The Author requests extraction of remaining resident diagnostics to external
utilities because installed EMOS has only 270 ROM bytes free. Move VDPTEXT's
algorithm/reporting onto SD and remove fake-adapter scaffolding from ordinary
firmware. Keep the existing minimal UART diagnostic gateway and cleanup.
Extender DIAG-003 owns the cross-component scope and compatibility record.

## State and authority

1. Authorized 2026-10-30: publish task and existing dirt, commit/push, then
   implement. This component checkout is clean at plan creation. Source belongs
   here; generic MOS build machinery and official references remain unchanged.
2. Baseline is the retained installed controller-enabled v0.1.27 composition:
   130,802 ROM bytes / 270 free. Build offline candidates as explicitly
   UNVERSIONED-DO-NOT-DEPLOY; do not assign a new release version implicitly.
3. No installed firmware, P4, startup, SD or running application is changed by
   this tranche. No emulator invocation is required. Emulator-coupled changes
   remain uncommitted pending explicit Author validation.

## Decisions

| ID | State | Decision |
|---|---|---|
| I17-01 | Author accepted | External utilities own diagnostic algorithms/reporting where practical; resident EMOS retains admitted transport ownership and cleanup. |
| I17-02 | Selected implementation | Add /emos/vdptext.bin through existing utility dispatch and ext.uartdiag v1. Keep fixed text/poll exchange and finite deadlines; no new resident service operations. |
| I17-03 | Selected implementation | Remove edu.text-probe and ordinary fake-adapter selection/branches. Document diagnostic compatibility retirement and keep external model tests; do not introduce a fake-route utility requiring a new firmware hook. |
| I17-04 | Constraint | No CRC/substitute checksum, application UART/GPIO bypass, controller wire change or unrelated parallel/firmware rewrite. |

## Bounded research

Official agon-docs/docs/mos/Executables.md and API.md are read-only references
at the workspace root. MOSlets load at B0000 and must fit the 32 KiB slot; the
ordinary executable header does not encode/prove that address. Existing EMOS
utility dispatch explicitly selects B0000 and rejects nested utility loading.
Existing ext.uartdiag admits only Legacy MOSlet callers and owns UART settings,
RTS, acquisition refusal and abandoned-lease cleanup.

Relevant maintained sources: src/emos.c, src/emos_uart_probe.c,
src/emos_uart_flow.c; projects/uartprobe and projects/uartflow. Retained installed
text source matches the current source. VDPTEXT sends its fixed visible text,
then 23,0,CA and 23,0,80,A6; expects 80,1,A6. TX/RX/quiet limits are nominal
120/600/24 MOS clock units with the existing stopped-clock fuse. Keep timing
units explicit; gateway overhead changes actual polling cadence.

## Implementation and validation

1. [x] Commit/push this plan before source changes; preserve exact baseline
   binaries/maps/provenance before reusing builder directories.
2. [x] Move visible text exchange and reporting to projects/uartprobe, build
   vdptext as a third utility, reuse the existing UARTFLOW gateway adapter and
   clock helper. Keep utility arguments/version output and failure cleanup.
3. [x] Remove resident text algorithms, gateway/CLI special cases and build
   inclusion. Keep ext.uartdiag transport/admission and its utility-exit cleanup.
4. [x] Remove fake-adapter command/state-machine branches from ordinary EMOS;
   preserve admitted real console routing and failure/recovery behavior. Refuse
   the retired FAKE name explicitly. Update real mode/gateway tests, retaining
   external model tests and frozen emulator evidence.
5. [x] Update guards/tests and current contracts. Retire historical consumers of
   edu.text-probe explicitly instead of silently redirecting them. Keep legacy
   emulator scripts frozen pending their own reviewed migration.
6. [x] Build all affected utilities and ordinary/controller complete ROMs via
   maintained wrappers and mandatory linked guards; preserve exact source and
   artifact hashes and compare like-for-like identity/configuration.
7. [x] Exercise actual utility plus resident transport against meaningful UART
   models: success, malformed/partial/extra replies, backpressure, deadlines,
   stopped clock, ownership refusal and cleanup. Run gateway/mode/keyboard/SD
   regressions; retain physical and emulator qualification as unperformed.
8. [x] Record measured linked ROM/RAM savings, utility slot bounds and all
   validation limits in the dated development log and Extender task record.

## Implementation note

Removing the resident text API also makes `emos_keyboard_text` unreachable.
Remove that private diagnostic wrapper and its declaration; retain the same
keyboard module's ordered readiness polling for real layout/admission work.
The host keyboard test exercises an interleaved key and release during the
retained layout poll. Historical browser-typing/visible-text samples and fake
emulator scripts remain frozen and require their original firmware; migrating
those emulator fixtures needs a separately reviewed validation gate.

## Offline result — 2026-10-30

Implementation is ready for physical review; no deployment/acceptance is claimed.
Both measured profiles use the same UNVERSIONED-DO-NOT-DEPLOY identity, pinned
builder/toolchain and configuration before/after. Controller ROM falls from
130,764 to 127,817 bytes (3,255 free); ordinary falls from 128,467 to 125,520
(5,552 free). Net saving is 2,947 bytes in either composition. Static RAM stays
4,189 controller / 4,058 ordinary bytes. The installed named v0.1.27 remains
130,802 / 270 free; its 38 additional identity bytes are not counted as savings.

Object recovery is text service 1,587 + gateway/command/mode 951 + unused keyboard
text writer 409 bytes. The admitted UART diagnostic gateway remains 538 bytes;
the controller module remains 2,199. VDPTEXT utility is 9,236 file bytes and
124 static RAM bytes; heap/stack room is 23,408 bytes, without a worst-case stack
claim. UARTTEST/VDPPOLL remain 9,523/9,522 file bytes.

All 145 repository host tests and 42 Extender controller tests pass, including
42 text-transport scenarios and real keyboard interleaving/cleanup checks.
Both complete wrappers and mandatory link guards pass. Linked instruction
checks pass controller overlap/Escape, 298 UART diagnostic and 21 reservation
cases. Host TX setup now includes the already-required inert startup binding;
the assembly-inventory test now includes four already-maintained private leaves.
Those stale test assumptions were corrected without new product behavior.

Exact baseline/candidate artifacts, maps, provenance, source snapshots and logs
remain in Extender ignored local evidence. The portable measurements and
compatibility limits are tracked in Extender docs/tasks/DIAG-003/RESULTS.md
and results.json. The old text-probe/browser-typing/fake emulator fixtures are
not valid against the new source. No emulator was launched or modified; no
firmware, startup, SD or physical device was changed. A separately identified
installation and physical utility/keyboard/controller review remain before
feature acceptance or production promotion.
