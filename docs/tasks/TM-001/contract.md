# Experimental single-buffer UART0 contract

Implementation base: Eirene86b694260e3be3b391284ce4aa8ecc758f7f34e9.
Scope: minimal Tom-derived sender on this fork; no public/upstream/EMOS ABI
allocation is claimed. Ordinary RST18 remains synchronous and unchanged.

## Existing facilities and chosen seam

At this commit src/mos_api.asm has unused entries42h and43h. Its getfunction
list ends at11h; neither table exposes the synthetic-receive syscall Tom
mentioned for his other MOS work. The stock _uart0_handler calls UART0_serial_RX
and vdp_protocol. Chain RX interrupts to that existing handler rather than
implementing a second parser. Reuse the existing RAM interrupt jump table;
install the complete state/vector atomically under DI, only after explicit
submission, and restore the stock handler on finish. Startup code is untouched.

Unlike the earlier prototype, an input-only user keyboard hook can remain
installed: this sender chains ordinary receive service. Such hooks must not
write UART0, enable interrupts or submit transfers from interrupt context.
This is a scoped contract, not a claim that arbitrary existing hooks are safe.

## Calls

Both calls use RST.LIL08h with ADL=1, MBASE=0, interrupts enabled, foreground
only. DI callers and callback reentry are rejected with CONTEXT; NMI use is
unsupported. Primary inputs, IX/IY and shadow banks are preserved; A=status,
flags unspecified. Caller must know it is running this experimental firmware;
the prior UTX1 table does not identify this interface.

| A on entry | Operation | Inputs |
|---|---|---|
|42h|Borrow one source for asynchronous VDP output|HL=source, BC=positive24-bit length, DE=callback or0|
|43h|Try to finish the session|No inputs; returns BUSY until source consumed and wire TEMT|

Status:0=OK,1=BUSY,2=INVALID,3=CONTEXT. BUSY/INVALID do not accept this source.
No queue, copying, delimiter mode, ticket or event record exists. While a source
is pending, another write returns BUSY without changing it. After its callback,
a new source may be submitted even if the previous FIFO tail is still leaving.

Like ordinary RST18, the caller must supply a readable contiguous source with
no address wrap and a valid callable callback (if nonzero). RAM and ROM sources
are allowed; no pointer-range or ownership-validation framework is added. Keep
source storage immutable and callback code live through completion. The UART must be
configured for its normal MOS receive/CTS mode; initial activation refuses
unfinished ordinary output, another UART handler, or conflicting TX/modem IRQ
ownership. Existing RX and optional line-status enables are retained.

## Completion, source ownership and callbacks

Initialize the application completion flag BEFORE calling write. It is possible
for completion to be visible before the caller's next foreground instruction.
On successful acceptance, retain immutable source storage until the callback;
a NULL callback is allowed, but then use finish as the conservative lifetime
check. A pending timeout does not free an accepted source.

The direct callback receives A=0, no other defined arguments, and runs with DI
on the UART interrupt's stack. It may be an ordinary bounded C function that
only posts application state. MOS saves main registers, indexes and shadow
banks. Callback must preserve ADL/MBASE, leave DI and return normally; no output,
SD calls, waiting, recursive MOS calls or task execution inside it. Do substantial
work in the foreground. A private assembly/C ABI bridge is needed to invoke
these register-ABI syscalls from ordinary C.

Completion means the last source byte was written to the UART FIFO. It is NOT
wire drain or receiver execution. Successful finish additionally confirms TEMT,
restores the previous baseline interrupt enables and the stock UART handler.
A receiver fence/readback remains necessary when command execution matters.

The aware app is the sole foreground UART producer during a pending write:
no RST18/PUTCH/printf or competing application UART ISR may interleave bytes.
Finish before returning, launching another application, replacing a handler or
reusing callback code. This minimal version does not install an automatic
application-return/crash framework. Correct explicit teardown is part of the
caller contract; no implicit compatibility guarantee is claimed for misuse.

## CTS and refill

One ISR service sends at most min(16,remaining) bytes. It checks FIFO-empty and
CTS before that block, then uses one OTIRX and updates pointer/count once.
No per-byte CTS checks are added. If CTS blocks, mask TX-empty (avoiding an IRQ
storm), retain modem-change wakeup and return; reconcile CTS after masking so a
recovery edge cannot be lost. No DI spin or timer polling is used. Modem events
are acknowledged even if the FIFO is not yet empty. RX remains serviced by MOS.

The scalar control replaces only the block copy with scalar loads/OUT0; it
retains identical block accounting and control flow. It is a lightly fixed
Tom-derived control, not the byte-for-byte original helper. Original code and
all differences are preserved in this task's reference directory.

No claim of receiver reserve, hardware performance or release readiness follows
from the16-byte setting or from a successful build. Flexe functional checks and
matched physical performance tests remain required.
