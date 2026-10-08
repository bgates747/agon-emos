# INTEG-016 — Bidirectional ExExt parallel transport

## Executive summary

Author authorized bench-free EMOS/EDP development on 2026-10-08. Extender
PORT-008 F02 owns the paired contract and eleven-wire P4-PC mapping. EMOS
owns admission, its Port C direction, PD5 clock/PD7 VALID and UART handover.
No physical deployment is authorized while the bench is occupied.

I16-01 [x] — Add a bounded private reverse receive reference to the existing
parallel engine, retaining the forward path. Host and target-build checks,
then CPU/emulator checks where available. This does not activate ExExt.

I16-02 [ ] — Under PORT-008 F02c, implement coordinator-owned negotiated
admission, UART drain/ISR fencing, direction release acknowledgements and
boot/reset recovery. Add the assembly hot loop using proved sender idioms.
No application bypass or public raw GPIO API.

I16-02a [x] — Current F02c1 increment: private four-byte handover sequencer,
with completed-adapter inputs and bounded transition calls. Test entry/return,
release failure and reset/cancel against the P4 counterpart, then execute the
linked C code in the eZ80 interpreter. No live GPIO/UART calls or boot changes.

I16-02b [ ] — F02c2: bind real UART packet drain, serializer/ISR fencing,
coordinator-owned matching admission and reset fences. Preserve queues and key
state during normal suspension; retained RAM is not promised across a reset.

I16-02c [ ] — F02c3: assembly payload loop and current direct-wiring adapters,
with exact target-build and software fault checks before the hardware gate.

I16-03 [ ] — When separately authorized, qualify exact bytes, first/last edge,
reset/timeout release and resumed keyboard input on hardware with Extender.
Do not infer electrical behavior or throughput from emulation.

## References and constraints

Official agon-docs/docs/GPIO.md and Zilog PS0153 define the GPIO contract.
Existing src/emos_parallel_engine.c, emos_parallel_io.asm and the retained
legacy PRX-06 sender provide implementation precedent. Cross-component
contract: sibling agon-extender/docs/tasks/PORT-008/BIDIRECTIONAL-DEVELOPMENT.md.
The reverse C loop is a correctness reference; pins/activation remain unbound.
Any failed read invalidates the entire output buffer; no partial-success ABI.
Emulator-coupled changes remain uncommitted until Author validation.

## 2026-10-08 bench-free result

Private reverse reference implemented in `src/emos_parallel_engine.c` with no
hardware binding or public API. Existing forward engine/binding/UART guards
pass 9 host tests; paired Extender tests execute both maintained endpoint
cores and pass 48 cases. Full wrapper `firmware-check` passes mandatory link
checks. The linked eZ80 image passes 38 instruction-emulator cases, including
IX/SP/IFF, byte/edge order, maximum block, invalid lengths, deadline wrap,
frozen ticks and sticky fault handling. No full-system Fab boot or physical
UART/GPIO timing claim. Image size 129,196 bytes, +574 against the retained
Pingo correction image; later assembly work must review this reference cost.

Reproduce CPU checks with the existing pinned test interpreter:

```sh
CARGO_TARGET_DIR=/tmp/emos-parallel-cpu cargo run --offline --release \
  --manifest-path tests/uart_put_cpu/Cargo.toml --bin parallel_reverse \
  -- LINKED_IMAGE_DIRECTORY
```

Supply the wrapper-built `MOS.bin` and its `nm.txt` in that directory. Host
paired checks live in sibling Extender `tests/parallel_reverse_test.py`.
Detailed cross-component result: sibling Extender
`docs/tasks/PORT-008/REVERSE-CORE-RESULTS.md`. No bench access, deployment,
commit or push; Author validation remains required for emulator-coupled work.

## Author safety-checkpoint authorization

The Author explicitly authorized a safety commit and push on 2026-10-08,
including the emulator-coupled development checkpoint. This supersedes the
earlier commit hold only. Hardware qualification, activation and production
promotion remain pending; no additional bench access is authorized.

## F02c1 handover result — 2026-10-08

The private handover state machine is compiled but has no live UART/GPIO
caller. Paired EMOS/P4 tests pass 5,208 cases, including unequal adapter delays,
reset/cancel at every step, failed release, blocked quiescence and retained
keyboard queue order. The new linked eZ80 state machine passes 16,392 ABI and
transition checks. Existing reverse-core host/CPU and forward/UART ownership
regressions pass, as does the canonical complete `firmware-check` wrapper.
Image 129,626 bytes: +430 bytes, 1,446 bytes below the image limit. IY is
caller-clobbered in the established AgonDev C ABI; IX/SP/IFF are checked.

The P4 candidate required an arming-cancellation guard: a negative control
without it reproduces an EMOS UART/P4 parallel-driver overlap after reset.
Extender's `docs/tasks/PORT-008/HANDOVER.md` specifies the exact candidate phases
and adapter contracts; `HANDOVER-RESULTS.md` and `HANDOVER-RESULT.json` retain
paired results and source/artifact hashes. The adapter must clear completion
bits for each new operation and on cancellation; a requested release is not
an acknowledged release. Only the existing EMOS coordinator may admit a
matching ExExt transaction. No actual wire opcode, boot fence, ISR/serializer
suspension or hardware activation is implemented by this increment.

No bench accessed or firmware deployed. New changes are uncommitted pending
Author review; the earlier safety-checkpoint permission did not automatically
approve this subsequent emulator-coupled tranche. Unrelated application-peer
work remains untouched.

The Author subsequently authorized committing and pushing the F02c1 checkpoint
before the next development tranche. Bench access remains prohibited; this is
not physical acceptance or production promotion.
