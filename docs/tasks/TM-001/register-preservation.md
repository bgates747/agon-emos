# Register preservation versus stock MOS

Normal MOS register preservation is not optional caller handholding. The
previous suggestion to remove it as such was too broad. Inspecting the actual
code confirms both the original official-MOS base and pristine Eirene use
per-function register preservation, with varying contracts; the RST08 dispatcher
does not save every register automatically.

| Boundary | Stock implementation | RC1 |
| --- | --- | --- |
| System call | LOAD/SAVE save BC/DE/HL; FOPEN/FCLOSE save BC/DE/HL/IX/IY | Submit/finish save BC/DE/HL/IX; IY/shadow banks are untouched |
| UART interrupt | Outer ISR saves AF/BC/DE/HL | TX service saves/restores the same main registers; RX chains to stock |
| User callback | Keyboard callback relies on the outer ISR's main-register saves; no extra IX/IY/shadow protection | Completion callback additionally saves IX/IY and both alternate banks |
| RST18 specifically | Advances HL, decrements BC and uses E/A/flags | New API preserves its inputs; this differs from RST18 but resembles other MOS calls |

Sources: src/mos_api.asm LOAD lines386..393 and FOPEN595..622;
src/interrupts.asm UART062..76; src/vdp_protocol.asm KEY172..181;
src/handlers.asm RST0815..16 and RST1831..48. These stock handler/parser files
are unchanged from Eirene86b694260e3be3b391284ce4aa8ecc758f7f34e9. The matching
original ZDS II MOS versions of mos_api.asm, interrupts.asm and vdp_protocol.asm
were also checked at that project's main9fd5aa1; the relevant save/callback
patterns agree. This is an actual-code comparison, not reliance on broad API
documentation promises.

## What is extra, and what is not

Our interrupt must restore registers touched by its own work or callback.
An asynchronous interrupt is not a foreground call whose register clobbers the
caller can simply plan around. Keep normal AF/BC/DE/HL interrupt preservation.
Tom's original EOF path restores BC/DE/HL before tail-calling the callback;
its supplied A-only callback fits, but a callback that clobbers those registers
would corrupt the interrupted caller. Keeping our main-register save active
until after the callback is a correctness property, not optional protection.

Saving syscall inputs is an ordinary MOS convention. There is no blanket
RST08 save that makes our local pushes redundant. The same stack frame also
holds our input arguments, so deleting pushes requires restructuring parameter
access, not merely deleting defensive code. A smaller implementation can share
or simplify wrappers while retaining its documented preservation contract.

The callback's extra IX/IY/alternate-bank wrapper is stronger than stock's
keyboard callback mechanism. It is a specific candidate for moving to an
application wrapper or narrowing under a documented callback convention.
Which saves an ordinary C callback actually needs must follow the selected
compiler/library ABI; do not assume that every possible C callback requires
all these saves, or that none are required.

## Assembled accounting

The released RC1 object contains20bytes of direct PUSH/POP instructions across
the two syscall entry wrappers (10each). These are comparable to ordinary MOS
per-call preservation, not20bytes of demonstrated removable excess. IX frame
setup and other wrapper instructions are not included in that figure.

The additional callback IX/IY/shadow-bank save/exchange/restore sequence costs
24instruction bytes. That count excludes callback dispatch, callback arguments,
NULL checking and the ordinary ISR saves. Moving it outside MOS is not a
measured24-byte net saving until the replacement contract/wrapper is built.
This limited accounting does not explain the entire318-byte difference from
Tom or justify removing all register handling. No firmware was changed, built
or executed for this audit; the released artifact/tag remain untouched.
