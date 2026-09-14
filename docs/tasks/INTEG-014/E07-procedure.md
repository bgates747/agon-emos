# E07 — Minimal UART transmit primitive comparison

## Executive summary

Retain E05's C block sender and deadline policy. Compare only its 106-byte
`uart1_keyboard_put` primitive with a stock-shaped assembly leaf. The E05
linked implementation calls frame setup, interrupt lock and unlock on every
byte; a leaf can preserve the same atomic operation without those calls.
E06 ruled out FIFO-trigger tuning as a material improvement. No receive
policy, packet format, required service, clock or feature removal is proposed.

The Author resumed E07 and expressly declined debugging the failed normal
MOS flash. Preserve that incident; do not make its investigation a prerequisite.
Known-good EMOS/P4 recovery and full connected wiring are accepted. Normal
hardware voice is the attention cue, spoken emulator fallback only if needed.

## Frozen scope and gates

1. [x] Preserve current source/index/generated-output state and E05 binary
   baseline. Freeze this contract before firmware changes.
2. [ ] Implement the one-operation assembly alternative, leaving the original
   C routine as a host reference. Validate return codes and register/port effects
   against the actual linked E05 C implementation using the pinned eZ80 CPU
   interpreter; include every LSR byte, CTS/THRE combinations, ownership/fault,
   both IFF states, representative byte values, stack and IX/IY preservation.
   This tests instructions, not physical UART timing. Keep the driver reference
   tests and sender timeout/partial/IRQ-off tests passing.
3. [ ] Build ordinary and bench compositions through canonical wrappers/link
   guards. Record complete-image and primitive sizes and linked calls. Reject
   unexplained ABI changes or image overflow. Retain C if there is no plausible
   material improvement; no wider assembly rewrite.
4. [ ] On hardware, establish current foreground/neutral input, end any running
   game cleanly, preserve current firmware/startup and reuse existing controlled
   UART endpoints and frozen pure-data fixture. Compare six large forward
   observations and three independent wire captures against E05. Repeat the
   existing full exact-byte and mixed-transfer controls for the candidate.
   The E04 5% material-time-reduction floor remains unchanged; unmeasured
   production/rendering claims remain out of scope.
5. [ ] Retain or reject the leaf from timing, correctness, size and ABI evidence.
   Keep RX and unrelated space changes separate. Restore agreed pre-run EMOS
   and diagnostic ESP images, verify flash where applicable and check actual
   CLI/keyboard/SD plus unchanged startup. Use maintained ZDI recovery if needed,
   never intentionally brick MOS to exercise it.
6. [ ] Summarize worst-first milliseconds/percent tables, limitations and exact
   manifests. Hardware voice, then stop at this E07 checkpoint before E08/E09.
   No experimental push; emulator-coupled changes remain subject to human review.

## Primitive contract

The existing callable C ABI takes a byte in a three-byte stack slot and returns
a byte in A. Preserve IX/IY, SP, alternate registers and caller interrupt enable
state. Under the same interrupt exclusion as the current function: check resident
ownership/fault before touching UART, read LSR once, give acknowledged line
errors precedence, stop peer/disable UART interrupts on error, check active-low
GPIO CTS, check THRE, and write exactly one THR byte only when ready. Preserve
all unrelated Port C bits. Return EMPTY0, READY1, ERROR2, UNAVAILABLE3, BLOCKED4.
No polling loop or deadline migration into the leaf; E05 retains those contracts.

## Evidence and references

[E03](E03.md), [E05](E05.md), [E06](E06.md), current `src/uart.c`,
`src/emos_keyboard_io.asm`, `src/emos_keyboard.c`, and stock MOS3.0.2
`src/serial.asm` define the reviewed comparison. Actual E05 bench symbols:
`_uart1_keyboard_put` at012968,106bytes; stop012910,21bytes. Exact images and
source hashes are retained under `build/integ-014/E05/block-retain/`.
CPU interpreter is upstream tomm/ez80 commit13c151e89f520b3bae3343bea9d67d06e8037cfa,
already pinned by official Fab1.2.4. A project-owned harness must not edit it.
The C/assembly return contract is local AgonDev's reviewed ABI, not an invented
ZDS parameter convention. Selected build identity and complete source manifests
must accompany each physical candidate.
