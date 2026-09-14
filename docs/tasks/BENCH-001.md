# BENCH-001 — Resident telemetry transport

Status: Active component work, authorized 2026-09-13.

The Author selected unattended resident telemetry, host-controlled Rally driving
and a speed-linked sine engine tone. The final physical driving run provides
the wake-up cue; all earlier checks are silent/headless. This supersedes old
planning-only restrictions for this bounded experiment, not unrelated work.

Implement the ext/telemetry gateway, Core-owned publication buffer, bounded
UART1 TX interrupt work and VBlank retry/timeout handling. Preserve all keyboard
RX effects, foreground packet boundaries, Core ownership and application exit.
No filesystem work or transient application pointer is retained by an ISR.

The cross-component execution checklist and provisional wire contract belong
to agon-extender/docs/tasks/BENCH-001.md and BENCH-001/TELEMETRY.md. This file
owns local implementation scope; it does not duplicate that execution register.
Use identified draft builds and preserved rollback. Necessary scoped deployment
uses the Author's standing firmware/SD/reset authorization; no commit or push is
requested. Preserve current branches and unrelated/concurrent work. Applicable
emulator human validation and commit approval remain separate from machine tests.

The explicit bench source profile is port/bench-telemetry.mk. It defines
EMOS_BENCH_TELEMETRY and excludes the old standalone UARTFLOW implementation,
with an explicit unavailable result if that command is requested. This makes
room in the 128 KiB flash image; default/full EMOS retains its prior diagnostics
and does not expose telemetry. Both profiles passed firmware-check and linked
checks locally. Exact dirty draft images and source hashes are in the owning
Extender agent's private bench bundle. Physical acceptance remains outstanding.

The Author subsequently requested faster driving with traffic avoidance. The
Extender-owned v2 contract expands copied snapshots to 140 bytes and defines
physics-tick overlap observations without gameplay collision response. Preserve
production physics and engine tone. Fresh game startup and held-Escape exit
bound each physical run; future attention cues use an emulator beep.
