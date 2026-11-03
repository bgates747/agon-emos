# Tom's candidate versus the experimental MOS sender

Tom demonstrates the essential idea in about100 lines of reusable assembly:
keep a source pointer and count, fill16bytes on a TX-empty interrupt, then call
a completion routine. That is a useful challenge to our design. Our1774lines
are not the minimum needed to send asynchronously. They also implement several
optional interfaces and accumulated experimental machinery. We should not make
an app pay for all that merely to send one immutable buffer in the background.

This is a source audit; functional/performance results are recorded separately.
Original attachment SHA256:93e5ddad8e72f1b22541c16e15cd0600e820204f02e13a559c2a6f6b423f5bd4.
Our audited source: d70b33a1bde375ceb29245ed7797e7226adefe7f (firmware build
inputs also match dcdcce19fe0b5d2da8d53f89c15f26cbb323fab5).

## Scope and size

Counts include comments/directives/labels and are not ROM-byte measurements.

| Source | Physical lines | Role |
|---|---:|---|
|Tom's whole attachment|163|Example caller plus reusable helper|
|Tom's helper/storage, lines58..163|106|One transfer, callback, interrupt chain|
|Our uart0_tx.inc|853|Low-level descriptor API, queue, CTS policies, RX/TX service|
|Our uart0_async.asm|297|MOS lifecycle, sync serialization, discovery and integration|
|Our uart0_completion.inc|624|Borrowed convenience API, events/callbacks, copied RST18|
|Our total|1774|All three layers, including optional features|

The major size difference is architectural, not an OTIRX implementation cost.
Tom checks CTS once per refill already; he does not check it before each byte.
His remaining FIFO loop still updates global pointer/count for EVERY byte.
Our grouped path accounts progress once per block, then OTIRX copies the block.
An OTIRX-only comparison must preserve this distinction: chunk accounting can
save cycles independently of the block instruction, as earlier experiments found.

## What each actually promises

| Concern | Tom as supplied | Our existing candidate |
|---|---|---|
|Pending writes|One global pointer/count/callback; caller must serialize|Three descriptors; explicit BUSY/backpressure|
|Source|Borrowed until last THR write|Borrowed/events/direct or optional copied RST18|
|Completion|One callback, no ID or context|Logical ID/context/event or direct callback|
|RX|Chains non-TX causes to prior MOS ISR|Services shared RX/TX, retains stock parser|
|CTS blocked|Spins inside DI ISR|Masks TX-empty, arms modem wakeup, returns|
|CPU work while FIFO draining|Yes, except ISR spin/copy|Yes, bounded service budget|
|Legacy output while active|No serialization against the new sender|Drains/serializes ordinary output|
|Function/child return|Caller must drain/unhook correctly|MOS fences borrowed sources before reuse|
|Copied-call convenience|None|Optional768B copy store and substantial extra code|
|Startup|App changes RAM vector after it runs|Explicit activation after unchanged stock startup|
|UART ownership|Replaces vector and chains previous handler|Checks ownership/keyboard-hook exclusion|

Neither source release promises UART TEMT or completed VDP commands. Tom's
helper can be useful under a narrow caller-owned contract; its absence of MOS
integration does not make that contract wrong. Conversely, missing queue or
copy semantics are not proof his narrower algorithm is defective.

## Definite problems/requirements in Tom's example

- Lines18/25 start transmission, then lines40..42 clear the completion flag.
  An early callback can be erased, leaving wait_done stuck. Arm before submit.
  This is a caller-example race, not proof the FIFO sender fails for long data.
- Lines108..115 spin with interrupts disabled. A long VDP pause blocks keyboard,
  RX and other timers; a permanent pause holds the CPU indefinitely. A software
  timeout driven by interrupts cannot rescue that ISR. Normal CTS flow control
  can cause the pause; it need not be an abusive input.
- Length0 still emits a byte then decrements0 to FFFFFFh. Caller must reject it.
- A second send overwrites a live source/count/callback; caller must serialize.
- Callback is unconditional; NULL is not a no-callback mode. BC/DE/HL are restored
  BEFORE tail-calling the callback. A callback that clobbers them can corrupt the
  interrupted foreground. The supplied A-only callback fits; a C callback needs
  an appropriate save/restore shim. Shadow/index registers need a clear contract.
- Init installs the new vector before recording the old one. If a non-TX UART
  interrupt arrives in that window, the chain operand is not initialized yet.
  A short critical section is enough; this is not an argument for a big framework.
- Exit restores the vector without independently draining or restoring TX IER.
  Its sample waits for completion (which disables TIE), but reusable callers
  must additionally respect wire drain before changing protocol/ownership.
- Raw foreground MOS output while a source is active can interleave bytes.
  The benchmark prints/writes files only outside that window.

## Concrete excess and poor code in ours

These findings are independent of whether Tom wins a timing comparison.

F-001 [ ] **Dead fixed-length copy fallback.** queue_copy_delimiter is reachable only
   after tx_write_delimited was nonzero. The flag remains invariant during the
   byte loop. queue_copy_fixed and its counted-byte fallback are unreachable;
   they survived the move of fixed writes to LDIR. Remove those branches and
   repeated mode checks. This is actual dead code, not a missing feature.
F-002 [ ] **Write-only fields.** Core descriptor original length at offsets13..15 is
   written on reserve/publish, never read. Completion metadata ticket at+5..7
   is written, never read. tx_write_ticket exists to feed that unused metadata
   and is also stored by the copied path without a reader. Removing all three
   can save21B of fixed RAM across the three slots, plus stores/layout code.
   Layout changes require map-based harness updates; the metadata ITSELF is used.
F-003 [ ] **Repeated searches.** reserve_core already returns IX pointing at the selected
   slot. Borrowed submission calls find, then publish_core calls find AGAIN.
   queue_reserve also repeats find after reserve. Preserve the selected slot
   inside the same masked operation rather than searching for it repeatedly.
F-004 [ ] **Dead flag-setting instructions.** reserve_core and publish's tail test each
   start with LD A,H / OR A,L, then overwrite A and recompute all flags before
   testing them. Remove the first pair in each sequence.
F-005 [ ] **Historical API layering.** The reserve/publish/status/cancel/reap interface
   is underneath a second convenience-ID/event layer. It is defensible for raw
   cancellation experiments but unnecessarily complex for a single-buffer app.
   A small borrowed writer could have one ID and one state record. Changing this
   is API/design work, not a safe deletion from the existing published table.
F-006 [ ] **Optional copied RST18 coupled to the main object.** It contributes a large
   portion of624lines plus768payload bytes even if callers only borrow. Consider
   a compile-time optional extension with accurate capability bits. The Author
   requested this interface; do not call it accidental or remove it silently.
F-007 [ ] **Hot-path state traffic.** Our block path reloads globals and repeatedly maps
   descriptor to separate metadata; full callback preservation adds stack work.
   Keep necessary preservation, but measure a narrow C-compatible callback shim
   and a flat one-transfer state before defending every existing save/load.
F-008 [ ] **Maintenance scaffolding.** Many internal labels are exported solely for model
   harnesses; isolated and production storage declarations are duplicated; runner
   scripts rewrite other scripts as strings and reference old build snapshots.
   These make the project harder to maintain. They mostly do not explain runtime
   CPU cost. Separate test symbol exposure and explicit runner configuration.

## Recommended direction

Measure Tom's one-buffer contract first. Keep a small single-buffer borrowed
sender as the design target, with normal RX handling, bounded CTS waiting,
explicit source completion and correct activation/teardown. Queue depth greater
than one, copied RST18, raw cancellation, diagnostic epochs and short-mode
compatibility are separate features with separate size/cost justification.
Keep our already-qualified image intact while proving reductions. Startup
remains untouched and upstream MOS modifications retain ZDS II/Wine.

Our claim should be that the larger prototype bought broader behavior while
finding the hardware/protocol constraints. It should not be that asynchronous
UART transmission inherently requires1800lines or that every prototype layer
belongs in the final implementation.

## Isolated cleanup build

The25-line patch was applied to a separate snapshot of the tested MOS and
compiled/linked with ZDS II/Wine. Its image is112724B versus112777B:53bytes
smaller. SHA2560f659bee6565dc6df150502fcced12a97f514ed73f80b3c46fe5ed91d758681d.
It has NOT been executed or deployed; assembly/link success is not functional
qualification. This patch does not yet remove the write-only fields or change
private layouts. Maintained sources and the cb1d0f4e comparison image are unchanged.
