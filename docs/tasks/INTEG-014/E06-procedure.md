# E06 — Receive and dual-UART diagnostic procedure

## Executive summary

Prepare a temporary application probe to count UART interrupts and capture
mainboard reply bytes while exercising the unchanged E05 firmware receiver.
Run it only after E05 passes its fixed physical comparison. This isolates
receive service and competing UART traffic without another EMOS rewrite or a
second baseline campaign. Original vectors, firmware and startup must be
restored before the spoken review checkpoint. No renderer or wiring change.

## Measurement boundary

1. Keep the E04 mainboard/P4 probes and selected E05 ROM. Bind the diagnostic
   application to that exact ROM hash and linked entry addresses; verify the
   ROM on hardware before use. Test-only internal calls are not supported
   application APIs or a production bypass around EMOS routing.
2. Use the documented `mos_setintvector` API to install temporary observation
   handlers within an admitted transfer interval. With interrupts masked,
   verify both current owners before installation and again before restoration.
   UART1's wrapper counts entries and immediately jumps to its original EMOS
   handler. No parser, FIFO, RTS or callback implementation is replaced there.
3. UART0's observation handler retains the current full register-save envelope,
   calls the original stock UART0 read routine and protocol parser, and copies
   each received byte into a bounded application-owned capture buffer. It also
   counts entries. The copy is needed because committed ExCom deliberately
   suppresses mainboard display/diagnostic effects. Never relax that production
   ownership gate simply to collect measurements.
4. No route, source, vector-owner transition or SD operation while observation
   handlers are installed. No VBlank/clock/keyboard suppression. Restore both
   vectors before returning to ordinary EMOS transitions and filesystem work.
   A foreign owner prevents restoration over that owner; retain failure and
   stop. Existing bounded transfer timeout provides the normal failure exit.
5. Probe work adds interrupt overhead. Report instrumented timings explicitly,
   compare like instrumented cases, and use E05's uninstrumented wire results
   for production-path timing. Count UART1 service pulses and their RTS-high
   durations from the existing wire captures; correlate with exact decoded
   byte/packet counts. Do not invent an unobserved FIFO occupancy histogram or
   UART0 wire timing. Report bytes per IRQ as an average where only totals exist.

## Bounded cases and correctness

1. First prove a quiet UART1 return and forward transfer with the observation
   handlers installed/restored and both original owners recovered. Use the
   existing deterministic PRNG and exact private probe replies.
2. Compare three loads with clock and required services always active: normal
   quiet UART0; ordinary VDP service queries during UART1 work; one controlled
   mainboard return burst of256records while UART1 work proceeds. The latter
   has4626framed bytes/2048useful bytes plus its terminal reply. Verify captured
   mainboard bytes, sequence, source and payload; P4 retains its existing exact
   callback verification. Keep query-production overhead visible in scope.
3. Use three repeats per selected return/forward case. For forward work, use
   identical bounded chunks in all cases so foreground code can issue service
   queries; do not compare these directly to E04's large-block timing as if
   chunk overhead were absent. Retain both IRQ counts and whole-call clock
   observations. If mixed traffic fails, preserve the first failure and inspect
   before changing a workload or timeout.
4. Where the same overlapping run supplies both ports' completion observations,
   report both with their actual resolution. A missing or unsafe converse
   comparison must be labelled rather than obtained by breaking routing/lease
   contracts. Any extra acquisition uses the existing verified P4 UART channels;
   no additional UART0 wiring is presumed available.
5. Compare measured work to E03's linked entry/save/parse/effect evidence. Try a
   minimal receiver alternative only if the observations justify a bounded
   change; preserve each independent experiment. Otherwise record the precise
   follow-up for E07 instead of proposing an unmeasured assembly speedup.
6. Show a short tests-complete/pass/fail summary on mainboard VGA, restore the
   agreed original physical firmware and startup, verify keyboard/CLI/SD,
   commit the evidence, play the standard spoken hardware cue and pause.

## References

Official MOS3.0.2 `docs/mos/API.md`, `0x14 mos_setintvector`, and tagged
`src/interrupts.asm`, `src/serial.asm`, `src/vdp_protocol.asm`; the maintained
EMOS IRQ envelope and ownership checks; E03 linked audit and E04/E05 exact
wire oracles. Official references remain read-only. Local build/evidence paths
and exact physical identities remain in ignored records.

## Failure isolation amendment — observer overhead

The corrected observer fails the first concurrent service/return case. It
records valid mainboard mode replies but incomplete P4 reception, then terminal
status15 and recovery35. Original vectors were restored; one explicit reset
recovered keyboard and SD access. Preserve this failure and its closed CSV.

Before testing a receiver alternative, run the same quiet and ordinary-service
cases with **both original IRQ owners left installed**. This `native` diagnostic
variant supplies18cases if all pass: three repeats of two loads and three
transfer sizes. It changes neither receiver nor timeout, and does not produce
an IRQ count or validate suppressed mainboard reply payloads. Label those
measurements unavailable, never zero-cost or error-free. P4 exact-data checking
and terminal recovery remain mandatory. A failed native run stops the matrix
for inspection; do not infer that the observer alone caused the first failure.

## Bounded FIFO alternative — admission after native comparison

If the original IRQ owners pass the native quiet/service comparison and normal
recovery, test one independent receiver configuration: change the owned UART1
receive FIFO trigger from one to four bytes, at the existing open/clear point.
Keep all IRQ/parser code, maximum16byte drain, software RTS, line-error checks,
timeouts and routing unchanged. Do not change an active FIFO, disable an IRQ,
or use a diagnostic application to alter UART registers. Build ordinary and
bench compositions through the canonical wrappers and verify the actual ROM.

Zilog [PS015317-0120](https://zilog.com/docs/ez80acclaim/PS0153.pdf), UART receiver
interrupts and Table59, specifies the trigger encodings and a receive timeout
after four byte times without FIFO activity. Data-ready and timeout share the
receive interrupt enable. Thus sub-four-byte packets must still be delivered;
verify this on hardware through keyboard admission, control replies and SD
round trips rather than treating the manual alone as qualification.

Run the unchanged full336 and mixed36 exact-data cases and three independent
wire4 captures. Compare their native return service/RTS measurements against
the existing E05 captures, using the frozen5% improvement floor. Repeat the
native quiet/service application with bindings to the new ROM. If either native
comparison fails, stop and retain evidence instead of declaring the FIFO
candidate safe. The longer instrumented burst is not an acceptance test for
unchanged receiver code when its observer has already invalidated the simpler
case. No E07 production selection is implied; original physical images still
return at this checkpoint.
