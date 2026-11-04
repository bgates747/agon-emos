# EMOS-001 — Isolated Eirene EMOS implementation

## Executive summary

Port the EMOS6ececd56 whole-file parallel/controller composition onto pinned
Eirene RC2 source825eaf7, preserving inherited APIs and mandatory linked checks.
The Author authorizes task-siloed offline development. No merge/deployment or
production selection is part of this tranche. Cross-component plan remains
in Extender docs/tasks/EMOS-001.md; this file owns firmware implementation.

## Work and gates

1. Native C and assembler are maintained in this dedicated branch, not generated
   builder outputs. Preserve upstream history/licenses and exact input manifest.
2. Reconcile core gateways, IRQ preservation, CLI editor/admission and UART1
   parking. The controller endpoint and SD upload leaf retain their existing
   layout; only active units link. Exclude retired payload/coordinator sources.
3. Retain normal blocking RST10/18. Explicit async42h/43h uses Eirene UART0
   OTIRX leaf; gate submission to mainboard route, and block route mutation
   until explicit finish restores IRQ ownership. Source/callback lifetimes
   and no-concurrent-output remain documented caller obligations.
4. Use `make firmware-check` with pinned toolchain; it must execute all four
   imported product link validators. Record each native-layout adaptation.
5. Compare baseline ROM, initialized RAM, BSS and available heap/stack reserve.
   Offline builds do not qualify hardware timing, receiver CTS margin or apps.
6. Stop for review before source integration, consumer changes or installation.

## Native adaptation notes

Eirene moved main.c to src/main.c; baud validator follows that actual maintained
file. GNU D24 replaces ZDS DW24 in the getfunction source spelling assertion.
All semantic linked checks stay enabled. Initial EMOS-only leaf conversion
uses the existing translator once, then promotes readable maintained source.
The temporary limited EMOS I/O adapter is isolated from native core numeric
register definitions and is a recorded migration debt, not new hardware logic.

## First native build checkpoint

The complete control and candidate compile/link on the pinned native compiler.
Candidate128098B leaves2974B in128KiB ROM. Divisor guard passes; the parallel
validator then rejects a legacy `_rst_10_handler` name absent in Eirene. Native
`rst_10_handler` and `rst_18_handler_0/1` are the actual retained gateways.
Only validator lookup names are adapted; exact call/routing checks remain.
A fresh admitted linked-check tranche must establish the remaining results.

## Offline result

All four mandatory linked checks pass after native handler-name and raw JR
displacement validator adaptations. Binary128098B SHA749ff6a8e0696d10e9efad62314d98a232c248e448911f00e81184ca5a6b7f30,
2974B ROMfree,802B initialized RAM/3482B BSS. Total static RAM4284B includes
Eirene RAM jump slots; old carrier total4174B includes its separate192B table.
No hardware/emulator or runtime compatibility pass is claimed. Exact complete
evidence remains in the cross-component EMOS-001 result/task-local archive.
