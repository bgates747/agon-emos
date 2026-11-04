# Using the fork's asynchronous UART0 sender

An aware app gives MOS one immutable buffer and continues its foreground work.
MOS's UART interrupt handler sends up to16bytes per refill using OTIRX, checking
CTS before each block. MOS continues handling incoming VDP/keyboard replies.
Ordinary RST18 remains synchronous. Nothing activates the new sender at boot.

The detailed [register contract](tasks/TM-001/contract.md) is frozen for published RC1.
The working tree now develops the smaller, unpublished [TRIM trial](../experiments/uart0-trim/contract.md).
Its caller preconditions replace CONTEXT checks, and raw callbacks preserve
IX/IY/alternate banks themselves or use a fixed-target application wrapper.
The following RC1 description is historical; use the trial contract and matching
example when building this branch.
This is a fork-local experimental ABI, not an upstream MOS syscall assignment.
Call only on the matching Eirene UART0 firmware, from foreground ADL1/MBASE0
with interrupts enabled. The example verifies the matching ROM before use.

| RST.LIL08h A | Operation | Inputs |
| --- | --- | --- |
|42h|Submit one borrowed buffer|HL=source, BC=positive24-bit length, DE=callback or0|
|43h|Try to finish and release UART ownership|No inputs|

A returns0=OK,1=BUSY,2=INVALID,3=CONTEXT. A BUSY write did not accept the new
source; a BUSY finish has not completed. Finish does not block. Retain source
and callback code while accepted, including during a timeout. There is no queue
or implicit cleanup when an application exits or crashes.

Arm completion state BEFORE submitting: even a short source can complete before
the first foreground instruction after the call. The callback means MOS has
consumed the source into the FIFO; it does not mean bytes have left the UART or
the VDP has executed commands. Finish additionally waits for local wire drain
and restores the normal MOS UART handler. A VDP acknowledgement/fence is needed
when receiver execution matters. A NULL callback is valid; finish then gives a
conservative source-lifetime check.

The callback runs with interrupts disabled on the UART interrupt stack. It may
post a flag/counter and return; no MOS calls, output, SD access, waiting, EI or
further submission there. In RC1, registers are saved by MOS; the trial moves extra callback saves into
the application wrapper as described above. Main work belongs in the
foreground. No other producer may write UART0 while the transfer owns it:
finish before printf, RST18, command output, returning or replacing handlers.
Input-only keyboard hooks may remain installed under the same ISR restrictions.

The [C example](../examples/uart0-async/) includes a thin register-ABI bridge.
Its globals are private call arguments, not a second queue or a source copy.
It checks ROM identity, sends one source, calculates while waiting and finishes
before printing. Treat it as single-foreground-caller code, not an ISR-safe or
reentrant general library. A larger convenience library can be built separately
once real applications establish what it needs.

Implementation provenance: Tom Morton's supplied async_vdu.asm, preserved with
its source hash in [TM-001](tasks/TM-001/README.md), and the upstream Eirene MOS
codebase. The maintained implementation is src/uart0_async.asm, with only two
new syscall table entries. See the task audit for the narrow correctness fixes.
All original upstream copyright/license notices are retained.
