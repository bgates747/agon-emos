# QUAL-001 — Qualify EMOS on physical Agon hardware

## State

- Status: In progress — the UART divisor correction is committed in canonical
  EMOS commit `0e24b06abdb322fdb4e681a21242ccfebfc8ea65`; its earlier dirty-source
  physical boot/smoke check against official VDP v2.16.0 passed. The former
  v0.1.0/fixed-forward predecessor is rejected. QUAL-002's clean v0.2.0
  candidate has now passed its ordinary hardware smoke on three cold boots,
  supported by the Author's photograph and report. Its installation-command
  deviation is recorded; full physical qualification remains pending.
- Started: 2026-08-31 18:32 EDT
- Finished: --

## Intent

Execute the recoverable, Author-operated physical qualification defined in
`docs/port-203-hardware.md` against a committed EMOS candidate and preserve a
hash-bound capture. Emulator success does not satisfy this task.

The Author's next small deployment milestone is tracked separately in
[QUAL-002](QUAL-002.md): a recoverable Legacy-mode EMOS boot and automatic
stock-compatible smoke test. Its bounded result may supply boot evidence here,
but does not complete this broader procedure or resume Extender transports.

The [ordinary boot result](../qualification/minimal-boot/runs/QUAL-002-2026-09-08-01-23-44Z/hardware-result.md)
identifies `agon-emos-v0.2.0-b2026-09-08-01-20-50Z`. It provides bounded
physical boot, SD-program, clock-progress and Legacy-state evidence, with
explicit observation limits. It does not close this task's remaining gates.

## Dependencies and gates

1. The candidate source, generic `mos-agondev` revision, generated firmware,
   stock VDP, recovery MOS, hardware profile, and procedure must have reviewed
   identities before deployment.
2. Real EDP transport stages remain blocked until `agon-extender` supplies a
   qualified transport and wiring profile; those cells may be skipped only
   with explicit reasons.
3. The existing hardware and emulator human-approval policies remain in force.

## Completion criteria

1. Every applicable procedure stage passes on real hardware.
2. Every skipped stage has a bounded reason.
3. Raw artifacts and hashes validate with the maintained capture validator.
4. The Author accepts the evidence and release claim.

## Historical boot-blocker correction

### Physical evidence

1. `agon-extender` run `PORT-008-2026-08-31-21-55-19Z` reproduced the original
   VDP-banner/no-MOS-banner failure with stock VDP v2.14.1 Dressing Gown still running.
   External ZDI placed the eZ80 in EMOS `_wait_ESP32`; General Poll completion
   byte `gp` remained zero while Timer 0 continued expiring normally.
2. Follow-up capture `PORT-008-2026-08-31-22-27-18Z` read UART0 BRG divisor
   `0x000B` from the same halt epoch and restored LCR and AF/MB. At the Agon's
   18.432 MHz system clock, official 1,152,000-baud operation requires divisor
   `0x0001`; divisor 11 is approximately 104,727 baud.
3. The source expression assigns into `UINT32` only after evaluating
   `CLOCK_DIVISOR_16 * pUART->baudRate`. Under AgonDev both operands use the
   eZ80 native 24-bit integer width, so `16 * 1,152,000` wraps from
   `0x01194000` to `0x00194000`. The subsequent division truncates
   `18,432,000 / 1,654,784` to 11, matching hardware exactly. This is a source
   portability defect exposed by AgonDev, not intended stock MOS behavior or
   presently evidence of an AgonDev compiler defect. AgonDev defines
   `__UINT24_TYPE__` as `unsigned int` with `__INT_WIDTH__` equal to 24, and its
   `stdint.h` maps that type to `uint24_t`; the wrap is standard unsigned
   arithmetic. The inherited code relied on the ZDS build producing the
   intended wider intermediate without expressing that requirement in C.

### Why Fab did not catch it

1. The exact Fab source used by the reviewed emulator, commit `fbb7d7c`, stores
   the programmed BRG divisor and uses it only to pace transmit and receive
   cooldowns in `agon-ez80-emulator/src/uart.rs`. It therefore made divisor 11
   communication slower than divisor 1.
2. Fab's CLI connects UART0 to the VDP with `ChannelSerialLink` in
   `agon-cli-emulator/src/main.rs`, which passes already-decoded bytes through
   in-process Rust channels. That VDP endpoint has no independent baud clock,
   sampler, framing detector, or baud-mismatch rejection. Both sides therefore
   continued to exchange correct bytes at the emulator's slower pace. Real
   hardware instead paired the approximately 104,727-baud eZ80 output with the
   VDP's fixed 1,152,000-baud receiver.
3. Fab's UART line-status model reports data-ready and transmit-empty state but
   does not synthesize the framing, parity, or overrun errors that the physical
   mismatch would cause. The existing boot and transcript tests had no linked
   BRG-divisor oracle, so their passing result was consistent with their stated
   functional—not physical timing—evidence boundary.

### Corrective work

1. Cast both operands to `UINT32` before the baud-product multiplication in
   both inherited UART0 and UART1 paths. Preserve all other upstream control
   flow and register writes.
2. Add a deterministic source/link regression that fails if the widening is
   removed or the selected build no longer programs the mathematically correct
   divisor.
3. Prepare a fresh named AgonDev worktree from maintained `agon-emos`, run the
   repository tests and fixed-profile firmware checks, and inspect the linked
   UART initializer.
4. Obtain the required Author emulator validation before committing any
   emulator-coupled correction.
5. Build and identify a replacement EMOS candidate, reinstall it through the
   keyboardless procedure, and repeat the physical boot gate before resuming
   broader PORT-008 transport work.

### Corrective execution record (2026-08-31 historical snapshot)

This subsection records the state of the boot-blocker correction when its
dirty-source diagnostic image was exercised. Later status is stated explicitly
where the original execution record is no longer current.

1. The inherited expression is changed only by widening both operands before
   multiplication in UART0 and UART1. No UART control flow, register sequence,
   timeout, packet, VDU, or EMOS service behavior was otherwise changed.
2. `projects/emos/verify_uart_baud.py` checks the two source expressions, the
   18.432 MHz / 1,152,000-baud divisor oracle, both linked 32-bit multiply and
   divide helper calls, rejection of the old 24-bit helper, and UART0/UART1 BRG
   writes. `tests/test_uart_baud.py` covers its source and fail-closed parsing
   behavior. The checker is part of the normal `linked-check` gate. Both EMOS
   source profiles also register it through `FIRMWARE_LINK_CHECKS`, so the
   generic repository-root `mos-agondev firmware-check` cannot build either
   supported EMOS profile without executing it against the final ELF.
3. A fresh named AgonDev worktree prepared from this dirty corrective source
   and `port/port008-forward.mk` passed all 54 `agon-emos` unit tests,
   `firmware-check`, `linked-check`, `port008-linked-check`, and module
   validation. Full `mos-agondev verify` passed 106 generic tests, custom MOS
   boot, shell parity, VDP regressions, formatter/API/FatFS contracts, and all
   build checks. The outer `qualify` target now propagates its selected
   `MOS_WORKTREE` into product provenance tests; this closes an earlier wrapper
   defect that could build one prepared tree while those tests inspected the
   default tree. The corrected end-to-end `qualify` invocation passes.
   This is historical predecessor evidence: INTEG-002 subsequently retired that
   source profile, and the same filename is now a fail-fast tombstone rather
   than a current qualification path.
4. The final scratch linked artifacts are `MOS.bin` (114,069 bytes, SHA-256
   `bf7633f9853e812d806a6528450d268db68541b12df0dcb47b4f68b15a130478`)
   and `MOS.elf` (247,284 bytes, SHA-256
   `b62f994bf2fc8bbc2821bdbf2a038d40ee70b8bb771583f093a5c081b3f41bab`).
   They are dirty-source diagnostics, not an identified deployment candidate.
5. The Author's 2026-08-31 graphical emulator check reached the normal MOS
   prompt, reported the expected EMOS identity and three providers, completed
   deterministic echo and separate EDU-result calls, and completed Legacy to
   Dual to Legacy mode transitions. This passes the required emulator gate.
6. At the time of this execution, the correction was uncommitted and the
   scratch image was unversioned. The correction was subsequently committed in
   canonical EMOS commit `0e24b06abdb322fdb4e681a21242ccfebfc8ea65`; the
   scratch image was never promoted into an identified candidate. QUAL-002
   subsequently established the approved v0.2.0 draft identity.
   A clean-source frozen build remains required before formal qualification.
7. Authorized `agon-extender` run `PORT-008-2026-08-31-23-11-18Z` used the P4
   as a temporary external ZDI programmer. It uploaded the exact 114,069-byte
   scratch image, executed the upstream `agon-recovery` flash agent, received
   its acknowledgement, and read back programmed eZ80 flash at CRC32
   `f202107f`. The P4 transcript is retained and hash-bound by that run.
8. After physical reset, the Author observed VDP v2.14.1 Dressing Gown and MOS
   3.0.2 Arthur. The established keyboardless smoke fixture discovered three
   providers, completed both service calls, entered fake Dual, returned to
   Legacy generation 2, and returned to the prompt without diagnostics or
   reset. This passes the bounded dirty-source boot-blocker check but does not
   satisfy this task's committed-candidate or full procedure requirements.
9. The onboard ESP32 was subsequently rebuilt and directly flashed from the
   clean official VDP v2.16.0 tag. Esptool verified all written segments. After
   an Agon reset, the Author observed VDP v2.16.0 Bistromathics and the same
   keyboardless fixture again passed discovery, both service calls, fake Dual,
   final Legacy generation 2, and prompt return without diagnostics or reset.
