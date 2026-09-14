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

The upstream interpreter revision is pinned to official Fab1.2.4's dependency;
offline mode requires it already cached. No upstream source is changed.
Inputs cover all256 LSR bytes, eight Port C patterns (including both CTS/RTS
states and unrelated bits), six byte values, three ownership/fault values and
both IFF states:221,184 cases per image. Instruction counts are descriptive,
not equivalent to eZ80 cycles or elapsed time. NMI delivery is not modeled.

The original C body in `src/uart.c` remains compiled by host driver tests under
`EMOS_UART_PUT_C_REFERENCE`. The target definition lives in
`src/emos_keyboard_io.asm`. The linked Port C writer guard admits exactly the
new error-stop label and one PC_DR write; it does not disable ownership checks.
