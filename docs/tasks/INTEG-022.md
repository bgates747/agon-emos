# INTEG-022 — Failure-only control TX attribution

Retain the first UART1 transmit failure before normal recovery changes pads.
This diagnostic composition supports Extender NET-004/N04-011 and is not a new
production API or transport remedy. Source/finite builds are selected by the
Smith-authorized MechAgon driver; physical actions require a stopped handoff.

I22-001 Publish the task before implementation. [x]

I22-002 Add a default-off diagnostic build flag and a private fixed RAM record.
Capture only status samples already taken by the native sender, original and
remaining count, first reason and elapsed/fuse state before error/recovery
writes. Retain first failure until reset, with no extra UART reads or logging. [ ]

I22-003 Reuse the existing foreground deadline helper rather than duplicating
console reply timeout arithmetic, retaining600-clock/262144-no-progress bounds.
Fit the complete matched ROM without dropping any feature or transport guard;
return concrete options before a feature tradeoff. [ ]

I22-004 Use normal compiler FIFO; run all six matched linked validators and
focused deadline/trace checks, and retain exact default/diagnostic ROM/BSS/maps.
Private fixture readers require the actual new map; public native198 does not
assume private snapshot addresses. [ ]

I22-005 Supply source/build/layout and stopping-point evidence to Extender.
No automatic flash, test replay, emulator or production promotion. [ ]

References: official MOS v3.0.2, official VDP v2.16.0, official docs
f9806bd3cbff6ed5d1c08bef1d51fed11764b86b (MOS UART1/API interrupt ownership);
matched native source c9c8b6be316b5ebb64da1f16773ec3c886349267;
src/emos_keyboard_io.asm, src/emos_keyboard.c and src/emos_console.c.
The historical first-fault record accepted0/fault_requested1 with600 elapsed
is consistent with control transmit failure, not proof of a particular wire
level, UART error, rendering defect or production correction.
