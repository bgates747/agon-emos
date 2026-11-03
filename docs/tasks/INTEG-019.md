# INTEG-019 — Retained parallel upload receive lease

EMOS retains one admitted shared-pin lease while the foreground file utility
receives small slices and writes Agon SD with CLOCK paused. The Author selects
whole-file P4 PSRAM on2026-11-03. Extender PORT-008/REVERSE-UPLOAD-R01 owns the
cross-component contract, held bench and finite qualification; this isolated
owner branch preserves the accepted sequential-reader implementation.

1. Reuse private ext.sdlink admission, keyboard serializer reservation, actual
   UART park/unpark, reciprocal handover and normal startup ownership. Add no
   global MOS API, application GPIO bypass or filesystem policy in resident ROM.
2. Candidate-only operations7/8/9 begin(total32), read(slice<=512) and end(abort)
   retain CLOCK/VALID and UART parking across application FatFS writes. EMOS
   accounts remaining bytes; no per-slice UART exchange or inactive tail clocks.
3. P4 hardware counters prove exact CLOCK/VALID before loop DMA retires. The
   ordinary shared-pin return still proves reciprocal release and UART restore.
   A file error must never activate a partial stage or replay an uncertain job.
4. Guard foreground/Legacy/manual SD lease, application ranges, IRQ state,
   current Port C/D and UART isolation. Utility abandonment fences ownership.
5. Keep CRC-free fast-v2 records and accepted reader reuse. New bulk capability
   and operation are explicit; old peers retain the UART path before admission.
6. Build through owner wrappers with all profile link/ABI/divisor/ROM checks.
   Execute linked raw receive/guard cases and foreground file-error checks.
   Record resident ROM/RAM and MOSlet sizes before a matched physical candidate.

Research baseline: official documentation f9806bd3, MOSv3.0.2/83364093,
VDPv2.16.0/c7ac293d remain read-only. MOS FatFS0x83 writes caller-owned slices;
PD5 clocks reverse Port C, PD4 READY aborts, PD7 VALID spans the whole payload.
P4 EOF-from-DMA is unavailable; ESP-IDF5.5.5 loop TX needs explicit stop.
No physical operation, performance claim or production change follows from code.

## Implementation and offline evidence — 2026-11-03

1. Private ext.sdlink operations7/8/9 and the foreground `--fast --parallel`
   selection are implemented. Without `--parallel`, the same utility binary
   remains the UART comparison. A zero-size begin query checks the private
   gateway before the utility advertises the bulk capability. No startup edit
   or automatic retry is introduced.
2. The selected profile reuses boot/handover and controller support, excluding
   the unused older native payload/coordinator. The owner firmware wrapper and
   every linked ABI/divisor/pad/ROM check pass:128880 ROM bytes,2192 free,3230
   BSS bytes. This is250 ROM and8 BSS bytes above the older native/controller
   profile; it is not the total parallel feature cost versus ordinary EMOS.
3. Actual linked receive instructions pass1034 modeled cases: lengths1..512,
   both interrupt states, exact edges and bytes, READY abort, register/stack
   preservation and no inactive tail. Paired actual coordinators pass15 cases;
   actual P4 engine plus utility file policy pass17, with no CRC function calls.
   These model CPU/peripheral boundaries, not physical throughput.
4. Boot handover intentionally retains a historical failure flag after recovery;
   begin clears it. New admission checks actual UART phase and current ownership
   before begin, preserving active fault guards and unrecovered-phase refusal.
5. Identified builds, MOSlet memory headroom and matched physical comparison
   remain required. Extender retains the bench; canonical utilities, production
   selection and normal startup are unchanged by this implementation.
