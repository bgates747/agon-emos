# Linked UART transmit comparison

## Executive summary

This headless instruction test executes two complete, linked MOS images at
their `uart1_keyboard_put` entry points against the same explicit port contract.
It checks return values, I/O sequence, interrupt exclusion/restoration, stack,
IX/IY and alternate registers. It is not a UART device model, interrupt-arrival
test, physical timing measurement or graphical Fab qualification.

Each input directory contains `MOS.bin` and the canonical build's `nm.txt`.
Pass the preserved pre-change C image first, then the assembly candidate:

```sh
CARGO_TARGET_DIR=build/uart-put-cpu cargo run --offline --release \
  --manifest-path tests/uart_put_cpu/Cargo.toml -- BASELINE_DIR CANDIDATE_DIR
```

The upstream interpreter revision is pinned to official Fab 1.2.4's dependency;
offline mode requires it already cached. No upstream source is changed.
Inputs cover all 256 LSR bytes, eight Port C patterns (including both CTS/RTS
states and unrelated bits), six byte values, three ownership/fault values and
both IFF states: 221,184 cases per image. Instruction counts are descriptive,
not equivalent to eZ80 cycles or elapsed time. NMI delivery is not modeled.

The original C body in `src/uart.c` remains compiled by host driver tests under
`EMOS_UART_PUT_C_REFERENCE`. The target definition lives in
`src/emos_keyboard_io.asm`. The linked Port C writer guard admits exactly the
new error-stop label and one PC_DR write; it does not disable ownership checks.

## E07P complete foreground sender

`cargo run --offline --release --manifest-path tests/uart_put_cpu/Cargo.toml
--bin block -- BASELINE_DIR CANDIDATE_DIR` executes `emos_keyboard_send`, not
just its byte leaf. It compares complete port histories, status/fault request,
clock read count and final private deadline. Its 119 cases cover 0..65,535-byte
payloads, clock wrap/deltas, valid accumulated deadlines and 24-bit budget
edges, CTS/THRE stalls, exact partial errors, source mutation during a stall,
and fault/ownership changes between the clock check and port attempt.

The harness models boundary state changes, not interrupt delivery/latency or
physical UART timing. Public C preserves IX/SP and alternate registers; IY is
caller-clobbered by the existing C implementation. TX01's private assembly
additionally saves IY. Public IRQ-disabled refusal and enabled restoration are
checked. The source data region is disjoint from the stack even at maximum
length. No new graphical emulator or runtime profile is involved.

## E07P receive byte comparison

`cargo run --offline --release --manifest-path tests/uart_put_cpu/Cargo.toml
--bin receive -- BASELINE_DIR CANDIDATE_DIR` compares the actual linked
`emos_keyboard_byte` implementations over all header/length combinations and
concatenated packets: 8,683,703 byte steps. Private state/payload stores, clock
reads, owner/fault guards, masked IRQs and IX/SP are compared after every byte.
Existing C effect/service call boundaries are recorded, with a deliberate key
callback edit; their real implementations remain covered by host and physical
tests. This does not model UART FIFO, ISR arrival or physical timing. The two
CPU decoders are reused with fully reset execution/register state per invocation.
The C parser is retained under `EMOS_RX_BYTE_C_REFERENCE`; its private six-byte
state plus 240-byte payload layout is size-checked alongside the target bridge.
