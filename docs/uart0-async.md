# Using the RC2 asynchronous UART0 sender

Give MOS one immutable buffer, then continue foreground work. MOS services TX
interrupts with up to16-byte OTIRX refills and a software CTS check per block.
Incoming VDP/keyboard replies continue through MOS. Normal startup and RST18
remain unchanged; applications explicitly use this fork-local interface.

RC2 is the smaller successor to RC1. Published RC1 remains the verified guarded
alternative, with extra context/ownership checks and callback register saves.
Do not assume RC1's stronger guarantees on RC2. No upstream syscall allocation
is claimed. Use only firmware matching your application's declared contract.

| RST.LIL08h A | Operation | Inputs |
| --- | --- | --- |
|42h|Submit one borrowed buffer|HL=source, BC=positive24-bit length, DE=callback or0|
|43h|Try to finish and release ownership|none|

A returns0=OK,1=BUSY,2=INVALID. Input primary registers, IX/IY and alternate banks
are preserved by the calls; flags are unspecified. A rejected write accepts no
source. Positive24-bit lengths, final partial blocks and NULL callbacks work.
One buffer may be outstanding; there is no queue or implicit app-exit cleanup.

## Caller requirements

Call in foreground ADL1/MBASE0 with interrupts enabled, normal MOS UART setup
and sole UART0 ownership. RC2 no longer validates those assumptions or returns
CONTEXT3. Finish before printf, RST18, SD operations that print, returning, or
replacing UART handlers. Do not use a scheduling ISR as another UART producer.
Keep an accepted source immutable and its data/callback code alive until release.
A timeout does not authorize freeing accepted storage or returning.

Arm completion state BEFORE submission: even a short source can finish before
the next foreground instruction. Completion means source bytes have entered the
FIFO, not that the wire drained or VDP executed commands. Finish additionally
waits nonblockingly for local transmitter-empty and restores the stock handler.
Use an appropriate VDP reply/fence when receiver execution matters.

## C recipe

The package's example/ is a complete working template. Its u0_submit/u0_finish
bridge takes C arguments and loads the register ABI. Its fixed-target assembly
callback gate preserves IX/IY/alternate banks around the C callback. The raw MOS
callback preserves AF/BC/DE/HL only. Each C callback needs its own gate symbol;
never overwrite a mutable dispatch slot on a write that might return BUSY.

```c
static volatile uint8_t released;
void complete(void) { released = 1; }
extern void complete_gate(void); /* fixed assembly gate calls complete */

released = 0;
uint8_t rc = u0_submit(buffer, length, complete_gate);
if (rc == U0_OK) {
    while (!released) useful_work();
    while (u0_finish() == U0_BUSY) useful_work();
    /* Now ordinary output and normal return are permitted. */
}
```

The example includes the full return-status handling and exact-ROM guard.
The bridge uses private argument globals and is single-foreground-caller code,
not a reentrant/ISR-safe general library. Copy/adapt its .c/.src/.h/.inc files
and retain the gate's explicit ADL1 declaration. The optional gate costs29bytes;
a raw assembly callback can instead preserve only the extra registers it uses.

Callbacks run DI on the interrupt stack. They may post flags/counters and return;
no output, MOS calls, allocation, waiting, EI or nested submission. Raw callbacks
must preserve IX/IY/alternate banks if touched and retain ADL1/MBASE0. Input-only
keyboard hooks may remain installed under the same ISR restrictions.

## Worked tests and build

The release ZIP contains full frozen MOS source, matching generated functional
and C-example sources, plus separate stock/RC1/RC2 PERF2 program sources and
binaries. They are recipes for receiver comparison, work overlap, pacing and
PRT timing as well as regression tests. See the ZIP's root README for commands,
identities, installation, expected PASS markers and interpretation of metrics.
Eirene and these original apps use AgonDev with the pinned compiler/libraries.
No private VDP protocol is needed. Stock Bistromathics2.16.0 was used for the
matched physical comparison. RC2 branding is the only MOS change afterward;
see the release build record rather than assuming exact branded-image testing.

Based on Tom Morton's Eirene and supplied async_vdu.asm. Original reference,
hashes, attribution and historical audits remain in docs/tasks/TM-001/. Preserve
all original MOS/FatFS/library notices and the project's MIT license.
