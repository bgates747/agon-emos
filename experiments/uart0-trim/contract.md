# TRIM trial contract delta from published RC1

The raw42h submit and43h finish register interface is unchanged: HL source,
BC positive24-bit count, DE callback or0. Both preserve primary input registers,
IX/IY and alternate banks; A=status, flags unspecified. Status0OK,1BUSY,2INVALID
are retained. Zero-length and a second pending write are still refused; NULL
callback is still supported. There is still only one immutable borrowed source.

Foreground ADL1/MBASE0/EI, normal MOS UART setup and sole ownership are now
caller preconditions. The former CONTEXT3 and foreign-owner/configuration checks
are removed; forbidden calls have no promised outcome. RX/line-status enables
are preserved; the sender owns only IER TX-empty/modem bits. Finish clears only
those bits, waits nonblockingly for source-empty/TEMT and restores stock RX
handler. Under this contract it is valid before the first submit or after an
already finished transfer. It is not an arbitrary UART-owner restoration API.

The raw callback has stock-keyboard-hook-style obligations: it must preserve
IX/IY/alternate banks if it touches them, keep DI/ADL1/MBASE0, and return normally.
The UART ISR preserves AF/BC/DE/HL through the callback. The source-release/wire-
drain distinction and no output/MOS reentry/waiting in callbacks are unchanged.

callback_gate.inc moves the optional extra register coverage into an app-side
assembly wrapper with a FIXED callback target. Each C callback gets its own
wrapper symbol. Do not implement a single mutable dispatch slot overwritten
by each attempted submit: BUSY rejection must not replace an active callback,
and completion may occur before the instruction after submit. Fixtures/examples
include a fixed-target wrapper; their bytes/cost are counted outside MOS.
No gate is needed for a raw assembly callback that preserves its touched state.

Normal startup, RST18, receive parser, full24-bit final-block accounting and
bounded CTS deferral remain. Published RC1 has the stronger old callback/context
contract and is unchanged. Trial branding and exact-ROM fixture guards separate
these images. Do not deploy existing RC1-only apps against the new raw contract
without adapting their callback convention.
