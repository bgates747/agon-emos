# Assembled size: Tom's supplied helper versus RC1

The released sender occupies476instruction bytes, compared with158instruction
bytes in Tom's original helper. That is318extra code bytes, about3.01times the
original. RC1's total net MOS image increase over pristine Eirene is480bytes,
including the4-byte net string/branding change; persistent RAM increases11bytes.

| Measurement | Tom's original helper | Released RC1 |
| --- | ---: | ---: |
| Instruction bytes |158|476|
| Inline reserved variable bytes |9|0|
| Separate BSS bytes |0|11|
| Assembled reusable block, excluding separate BSS |167|476|
| Persistent writable state |12|11|

Tom stores source/count/callback in9inline reserved bytes. He also saves the
previous ISR address in the3-byte operand of a JP instruction. Those3bytes
are writable state already included in the158instruction-byte count, so do
not add12again to167. His helper runs in application RAM and modifies its own
trampoline; it is not a ready MOS-ROM integration. Our11bytes are separate MOS
BSS; code remains in ROM. These persistent-state figures exclude stack usage.

The additional code implements explicit submit/finish syscall behavior,
context/length/BUSY and ownership checks, safe bounded C callback context,
nonspinning CTS/modem wakeup and explicit drained teardown. Tom's supplied
scalar helper is narrower, spins with interrupts disabled while CTS blocks and
has no OTIRX. This is not a comparison with the later lightly fixed scalar
control used in performance tests. The added code is not primarily OTIRX size.
No queue, copied RST18 or application-exit recovery framework was reintroduced.

Tom's exact original attachment is preserved in reference/async_vdu.asm,
SHA25693e5ddad8e72f1b22541c16e15cd0600e820204f02e13a559c2a6f6b423f5bd4.
For measurement, GNU assembler syntax was substituted only for equates,
directives, scoped labels, expression brackets and hex literals. No algorithm
or instruction optimization/fix was applied. The full example also maps its
byte-boundary align64 directive to GNU .balign64; its complete947bytes include
caller, header/alignment and Lorem Ipsum data, so167is the meaningful reusable
helper size. That helper's span in the full example is independently167bytes.

The selected same AgonDev assembler/linker on Oryx assembled both helper-only
and full-demo files with ADL1 and -march=ez80+full. Link address was0x40000;
no emulator process, hardware execution or flashing occurred. RC1's map reports
.text0x1dc and .bss0xb for obj/uart0_async.o; the syscall table reuses existing
entries without enlarging it. Whole images are100660and101140bytes.

Normalized source, binaries, object files, map, disassembly and provenance are
preserved on Pi and Oryx under build/tm001/tom-size001. [Measurement pins](size-comparison.json).
These measurements do not alter the published RC/tag or its assets.
