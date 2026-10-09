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

I16-02b1 [x] — PORT-008 F02c2a: private real UART park/restore leaves,
serializer/Port C reservation, complete-packet and TEMT checks, RX isolation,
and parked-state TX/IRQ/RTS guards. Linked CPU and host tests plus the full
firmware wrapper pass. No live caller or ExExt activation.

I16-02b2 [ ] — PORT-008 F02c2b: bind the leaves to paired phase machines,
matching wire admission, bounded recovery and real startup/reset fencing.

I16-02c [ ] — F02c3: assembly payload loop and current direct-wiring adapters,
with exact target-build and software fault checks before the hardware gate.

I16-02c1 [x] — Private guarded native byte leaf and RAM-only instruction
tests pass. Full resident composition correctly refuses 265-byte ROM overflow;
parent integration and physical qualification remain open. See dated result below.

I16-02c2 [x] — Bounded ROM fit: exclude the uncalled reverse C reference from
ordinary/native residency while preserving an explicit reference-test profile.
Complete wrapper builds, actual native/reference instruction execution and
scoped linked-owner negative controls pass. Native composition now fits with
309 bytes free; live integration and physical qualification remain open.

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


## UART parking development — 2026-10-08

Private `emos_keyboard_parallel_park/unpark` reserve the existing serializer
and Port C lock, preserving keyboard state through normal suspension. UART
ownership value 2 keeps public access excluded while target TX/parser/IRQ
and C RTS/timer paths avoid shared pins. TEMT and a complete packet boundary
are mandatory; acknowledged errors remain pending for the normal fault ISR.
Zilog PS015317-0120 page 119 documents MCTL.LOOP's external RX disconnection;
UART interrupts disabled alone would not isolate parallel edges. Restore
requires all local data lanes input and coordinator proof of peer release.

289 linked eZ80 parking checks pass; regression comparisons pass 221,184 byte
TX cases/image, 375 block cases, 1,302 complete IRQ cases and 8,683,703 parser
cases. Host receiver/parallel/provenance checks and all firmware wrapper guards
pass. Exact Port C write sequences for the two new leaves are added to the
linked ownership inventory. The stale source-list test now includes the
preceding handover unit. Normal byte TX adds two interpreted instructions.
Image: 130,135 bytes, +509 over the preceding checkpoint, 937 bytes remaining.
These are model/build checks, not target throughput or a complete Fab boot.

Paired P4 adapter/build evidence and remaining gates:
`../agon-extender/docs/tasks/PORT-008/UART-PARKING-RESULTS.md`. Existing ordinary
UART startup still runs; neither new leaf is bound to an activation path.
No bench operations occurred. New changes remain uncommitted for Author review.
Unrelated `scripts/application_peer.py` is untouched.

The Author approved committing and pushing the UART parking checkpoint before
continuing with F02c2b integration. This preserves bench-free development
evidence; it does not authorize deployment or imply production acceptance.


## Accepted reset policy and next admission increment

The Author requires reciprocal physical release after reset in the future
parallel-capable pair. EMOS keeps shared PC0–PC7 released if P4 is absent or
older; mainboard MOS/keyboard remain available. No timeout reclaims UART pins.
See Extender P08-F-D03 / ADR-0026. Current UART-only boot remains unchanged
until candidate startup integration; no claim that this fence is live today.

I16-02b2a [x] — Completed bounded first part of I16-02b2: implement the private version-2
block-offer/ack gate with session, sequence, exact length/direction and ExExt
checks before the existing handover sequencer. Reuse console CRC code; measure
ROM growth against 130,135 bytes / 937 bytes free. Execute linked eZ80 checks
and paired P4 checks. The remaining coordinator owns fresh-session negotiation,
serializer fencing, deadlines and boot binding; native payload remains I16-02c.
Protocol and cross-component evidence live in sibling Extender
`docs/tasks/PORT-008/PARALLEL-ADMISSION.md`. No bench access or deployment.


I16-02b2a result: 18,097 paired admission checks and 18,474 wrapper-linked eZ80
checks pass, covering all legal lengths/directions, both interrupt states, exact
acknowledgement, replay/exhaustion, ABI and memory guards. Existing console,
parallel, linked UART parking and prepared-source tests pass. All full EMOS
wrapper guards pass. Image 130,551 bytes (+416), 521 bytes free. The shared
console CRC has unchanged linked instructions after resolving helper addresses.
The P4 native build and its exact source check also pass. See sibling Extender
`docs/tasks/PORT-008/PARALLEL-ADMISSION-RESULTS.md` for hashes and complete scope.

These are private gates accepting trusted coordinator state, not a live session
exchange or startup fence. Remaining I16-02b2 integration must reconcile ROM cost
and bind the accepted mandatory release policy. Native payload remains I16-02c.
No deployment/bench access; new emulator-coupled changes remain uncommitted for
Author review. Unrelated application-peer work is preserved.


I16-02b2b [x] — Author-authorized next bounded integration: reserve the existing
serializer/Port C lock before a control offer, reuse bounded TX for a private
reserved sender, retain ownership through UART restore and explicit completion.
Reject nested sends/release/park and cancellation while physically parked.
Measure against the preceding 130,551-byte image (521 bytes free); test real
source and linked eZ80 code. P4 owner-core installation is owned by Extender.
No live session exchange, mode entry, boot pin fence or payload activation yet.
Existing uncommitted admission work remains preserved; the bench is unavailable.


I16-02b2b result: 22 linked reservation cases and 289 linked parking cases pass;
375 full sender and 1,302 IRQ comparisons preserve existing behavior. The actual
keyboard C harness passes ordinary and telemetry configurations, including
recursive callback refusals and failed/partial send handling. Full wrapper
guards pass. EMOS is 130,791 bytes (+240), leaving 281 bytes. The ready block
send adds 22 interpreted instructions per call; the sampled IRQ is unchanged.
P4 installs UART from its existing console owner core; its SDK/target checks pass.
See sibling Extender `docs/tasks/PORT-008/UART-RESERVATION-RESULTS.md`.

Current private caller order is reserve before offer, private bounded sends,
park/restore, final status or session abandonment, then explicit release. Busy
parking and successful restore retain the guard. No release while parked or
inside a send. Partial sends request deferred cleanup; ordinary send also
refuses pending fault/stop rather than appending bytes in that interval.
Remaining coordinator/boot work must reuse or replace code within ROM limits.
No physical operations; changes remain uncommitted for review.


I16-02b2R1 [x] — Extract UARTFLOW as a foreground EMOSlet, retaining the
admitted `ext.uartdiag` transport and lifecycle cleanup. Complete EMOS and utility
builds pass; the original 15 scenarios, boundary/adapter checks, 285 linked eZ80
diagnostic cases and 22 reservation cases pass. ROM is 129,464 bytes, leaving
1,608 free: 1,327 recovered including the new service cost. Static RAM drops by
9 bytes. UARTTEST/VDPPOLL and text-probe are unchanged. Hardware, full-system
utility loading and physical timing remain untested. No bench access or commit.
Details: [extraction results](../../../agon-extender/docs/tasks/PORT-008/UARTFLOW-RESULTS.md).


I16-02b2R1H [x] — Author released the bench for the extraction's hardware test.
Complete ROM and utility readbacks match. Real paired FLOW/ACK stop/resume and
blocked-return timeout/cancellation pass; Agon returns zero and saves clear
UART/RTS ownership. Wrong-peer control returns nonzero and reacquires input and
listener without reset. Original P4/startup restored, mainboard VDP unchanged;
candidate EMOS remains installed. Bench released. No waveform acquisition,
parallel activation, production promotion or commit. See sibling Extender
[hardware report](../../../agon-extender/docs/tasks/PORT-008/UARTFLOW-HARDWARE-RESULTS.md).


I16-02b2c [x] — Author-authorized private session lifecycle increment. Reuse the
fixed v2 carrier and console CRC/prepare/challenge/commit idiom, mirrored in P4.
EMOS requires UART-recovered handover state and its actual foreground serializer
reservation; only confirmed active capability can use the new admission wrapper.
Cancellation invalidates capability/sequence and requests release without dropping
the reservation or claiming pad cleanup. These functions have no live parser or
startup caller; fresh identities, stale-byte quarantine, deadlines and physical
boot fence remain I16-02b2 integration.

Full wrapper/link guards, 1,149 paired owner checks, 1,334 linked eZ80 session
checks, prior reservation/parking and affected host regressions pass. Image is
130,212 bytes (+748 versus UARTFLOW), leaving 860 free; static RAM remains 4,058.
Both P4-PC target build and source closure checks pass. No hardware/SD operations
or commit. [Results](../../../agon-extender/docs/tasks/PORT-008/PARALLEL-SESSION-RESULTS.md)
retain exact evidence and limitations. Unrelated application-peer work remains
untouched. Stop for Author review before live coordinator binding.


I16-02b2d [x] — Author-authorized control-owner binding. Reuse the console
request buffer, actual reserved sender, fixed FF/16 ISR dispatcher and bounded
wait for foreground prepare/commit. Reject borrowing an existing ExCom lease's
identity; capture only one matching reply. Failure invalidates capability and
requests release without dropping the reservation or restoring pads. No live
startup/mode caller or physical boot-release/payload activation.

218 paired control cases and 10 complete-image eZ80 control cases pass, with
prior session/ownership and console regressions. Both target wrappers and P4
source verification pass. EMOS is 130,480 ROM bytes (+268), leaving 592 free;
static RAM remains 4,058. No bench operation or commit.
[Results](../../../agon-extender/docs/tasks/PORT-008/PARALLEL-CONTROL-RESULTS.md)
retain exact evidence. Mandatory early boot pad fencing, fresh identities and
stale-wire quarantine, drains, block deadlines/status and payload binding remain.

I16-02b2e [x] — Author released the bench and authorized the next bench-free
boot-release increment. Add private physical boot leaves using the existing
Port D atomic helpers and handover state; verify actual linked port writes,
paired SDK/register behavior and complete guarded builds. No startup activation
or hardware operation. Extender PORT-008/BOOT-RELEASE.md owns the cross-owner
contract; actual earliest-writer gating, deadlines, identity/quarantine and
physical one-board-reset qualification remain parent integration gates.

I16-02b2e result: 695 paired physical boot-adapter cases and 126 complete-image
eZ80 boot cases pass, along with existing control/session/reservation/parking
regressions, six profile checks and both complete guarded target builds. EMOS
adds 238 bytes to 130,718 ROM bytes, leaving 354; static RAM stays 4,058. The
new linked guard permits only the boot leaf's exact Port C sequence and refuses
unreviewed direct startup calls. An injected-call negative control is rejected.
No ordinary boot binding, bench operation, production change or commit. Actual
F92 defaults confirm init_UART1 leaves pins input; open_UART1 is the first mux
enable to gate. Cross-owner results: Extender PORT-008/BOOT-RELEASE-RESULTS.md.

I16-02b2f [x] — Author-approved candidate startup binding, off-bench only.
Select inert hooks for ordinary UART-only startup and a separate private boot
coordinator for the candidate. Reuse existing keyboard deadline and transport
claim/release, gate actual open/restore mux writers and preserve mainboard MOS
on timeout. Run complete guarded builds and account for the 354-byte entry
headroom before P4 integration. Stop if it does not fit; no relaxed ROM guard
or additional extraction. Cross-owner contract: PORT-008/BOOT-STARTUP.md.

Startup binding results: complete guarded EMOS candidate fits at 130,949 ROM
bytes, 123 free; ordinary inert-hook composition is 130,747, 325 free. Actual
startup coordinator/claim/deadline/gates pass 12 linked instruction cases;
706 paired P4-adapter/coordinator cases and both target compositions pass.
The linked guard checks zero admission skips hardware writes and rejects removed
result tests. Prior UART/control/boot and sender regressions pass. No firmware
diagnostic, deployment, production change or automatic commit. Runtime reset,
fresh identity/quarantine and payload binding remain open. Cross-owner evidence:
Extender PORT-008/BOOT-STARTUP-RESULTS.md and BOOT-STARTUP-RESULT.json.

The Author subsequently authorized logically grouped commits and publication.
EMOS implementation/tests are preserved in `4af0792`; the owning task record
is committed separately. Original Extender evidence retains its pre-commit
identities and as-captured state. This is an off-bench source checkpoint, not
hardware acceptance or production promotion. Unrelated application-peer work
is excluded.


I16-02b2g [x] — Author-approved off-bench observed-release invalidation. Private
candidate admission latches loss of P4 READY; the existing keyboard tick, after
normal parked-owner exclusion, uses existing fault/vector cleanup and fences
the shared pins. No ISR wait, automatic reconnect or committed-route change.
Compile/account first against 123 ROM bytes free; stop on overflow before
further extraction or P4 work. Contract and linked-test gate:
[PORT-008 runtime release](../../../agon-extender/docs/tasks/PORT-008/RUNTIME-RELEASE.md).

Observed-loss results: 26 actual linked startup/runtime cases and four guard
negative controls pass; the prior candidate fails the READY-bounce regression
control. Existing keyboard/console/parallel/profile and linked parking/control/
boot-leaf checks pass. Final guarded candidate is 130,981 ROM bytes (+32),
91 free, static RAM unchanged at 4,063. Ordinary inert hooks use 130,752 ROM
bytes, 320 free. No P4 change, hardware operation, automatic recovery, production
change or commit. Fresh identity/quarantine, full runtime reset detection,
foreground reconnect and ExCom route recovery remain parent integration work.
[Results](../../../agon-extender/docs/tasks/PORT-008/RUNTIME-RELEASE-RESULTS.md)
and its machine-readable result preserve identities. Pause for Author review.

## 2026-10-09 native payload leaf

Private `src/emos_parallel_native.c` and `emos_parallel_native_io.asm` now
provide the guarded byte loop, selected only by
`port/parallel-native-candidate.mk`. The parent I16-02c remains open: no live
coordinator calls this leaf, and full ROM integration was refused as required.
The C guard adds 210 bytes and assembly 146; the composition would use 131337
ROM bytes, 265 beyond capacity. No oversized firmware was emitted or flashed.
The ordinary profile and installed firmware are unchanged.

124 actual compiled C/assembly instruction cases pass in a RAM-only image:
both directions, exact bytes/edges, bounds, IFF policy, unrelated Port D ISR
changes, READY loss, ownership/GPIO refusal, IX/SP and raw assembly IY.
The caller keeps received bytes provisional until matched UART completion;
this leaf does not replace the full admission/drain/deadline/status owner.
Physical first/last byte and contention tests remain outstanding.

Reproduce after the maintained private-profile compile/link attempt:

```sh
python3 tests/link_parallel_native.py --build-root MOS_PORT_BUILD_DIRECTORY \
  --toolchain AGONDEV_TOOLCHAIN_DIRECTORY --output FRESH_RAM_TEST_DIRECTORY
CARGO_TARGET_DIR=/tmp/emos-parallel-cpu cargo run --offline --release \
  --manifest-path tests/uart_put_cpu/Cargo.toml --bin parallel_native \
  -- FRESH_RAM_TEST_DIRECTORY
```

The generated payload loads at 0x40000 only in the instruction interpreter.
It is neither flashable firmware nor a MOS executable. Cross-component result:
sibling agon-extender/docs/tasks/PORT-008/NATIVE-PAYLOAD-RESULTS.md. Changes
remain uncommitted for Author review; unrelated scripts/application_peer.py
is untouched.

## 2026-10-09 native ROM-fit continuation

The Author committed/pushed the preceding payload checkpoint and authorized
ROM trimming. `EMOS_PARALLEL_RECEIVE_REFERENCE=1` now selects the maintained
uncalled C reference only for tests. Full ordinary EMOS uses 130178 ROM bytes
/894 free; the private native composition uses 130763 /309 free. Exactly 574
bytes were recovered; static RAM remains 4058/4063 respectively. The opt-in
reference composition remains 130752 bytes /320 free. No supported command or
public API was removed, and the forward engine is unchanged.

The first fitting native link correctly failed its required Port C inventory.
The private profile now selects `verify_parallel_native.py`, which reuses every
ordinary check and admits only the audited native write sites, sole guarded
raw caller and atomic Port D helper. Ordinary profiles reject native symbols.
Seven actual-ELF positive/refusal controls pass. Native instruction tests now
execute both the full resident image and the retained RAM-only composition
(124 cases each). Reference instruction tests pass 38 cases; startup/runtime
passes 26; forward/selection host tests pass 10; paired reference/boot tests
pass 48/718 cases. No electrical timing or throughput claim.

For the receive-reference instruction test, build the explicit
`port/parallel-reference-test.mk` composition through the normal mos-agondev
root wrapper and supply its retained `MOS.bin`/`nm.txt` directory to
`parallel_reverse`. Ordinary/native images deliberately lack that reference.
For native instructions, `parallel_native` accepts a directory containing the
full `MOS.bin`/`nm.txt`, or the separate RAM-only `payload.bin`/`nm.txt`.
`tests/check_parallel_native_link.py --help` describes linked-image regression
inputs; its pyelftools dependency is shared with the existing ROM accounting.

Complete paired result and source/artifact hashes:
[PORT-008 ROM fit](../../../agon-extender/docs/tasks/PORT-008/NATIVE-ROM-FIT-RESULTS.md).
New changes remain for Author review before commit. No bench access, flashing,
activation or promotion; unrelated scripts/application_peer.py stays untouched.


## 2026-10-09 private block control transaction

`emos_parallel_block_offer` and `emos_parallel_block_complete` reuse the existing
console request buffer, reserved UART sender, fixed FF/16 reply ISR and bounded
wait. ACK/result matching now includes session, sequence, length, direction,
CRC and bounded status. A matched payload failure is captured immediately and
rejected in foreground; uncertain delivery invalidates the session and requests
release recovery. A recorded local handover failure is sent as failure even if
the caller supplied success. No GPIO/native entry or public API was enabled.

P4 retains one provisioned buffer/descriptor and withholds its successful
receipt until completed UART return and matched local/peer results. Deadlines
and cancellation revoke pending work; reset revokes unread receipts. A matched
semantic failure after clean return keeps UART available for the failure reply.
The physical owner still must drain that reply; timeout is never release proof.

689 paired owner cases and 65 actual linked-eZ80 control cases pass, including
actual reserved UART TX, receive framer, reply ISR and scripted handover steps.
Ordinary complete firmware/link checks pass at 130677 ROM bytes /395 free and
4058 static RAM bytes. This adds 499 ROM bytes to the prior ROM-fit baseline.
The combined native image would use 131262 /190 over capacity; the unchanged
linker refuses it and emits no oversized firmware. Another bounded ROM recovery
is required before the live coordinator can be installed. Preserve the earlier
fitting native image; no bench/SD/flash/production change or commit this tranche.

The current paired contract/results are in sibling
agon-extender/docs/tasks/PORT-008/BLOCK-CONTROL.md and BLOCK-CONTROL-RESULTS.md.
Full native caller, actual UART drain/phase adapters, boot-monitor handoff and
hardware qualification remain I16-02c/PORT-008 integration gates. Unrelated
scripts/application_peer.py remains untouched.

## 2026-10-09 diagnostic MOSlet ROM recovery

Author authorized off-bench recovery while the bench remained occupied.
UARTTEST/VDPPOLL sequences and reporting now live in projects/uartprobe;
`EMOS UARTTEST` / `EMOS VDPPOLL` use the existing /emos loader. UARTFLOW's
adapter is shared; ext.uartdiag adds only operation 5 for fixed 115200/no-flow
acquisition. EMOS retains admission, nonblocking UART leaves, pin ownership,
cleanup and resident text services. No raw utility access or new loader.

Net recovery is 1938 ROM bytes, not counting previous UARTFLOW savings.
Ordinary 128739 /2333 free; combined native 129324 /1748 free; explicit
reference 129313 /1759 free. Static RAM remains 4058/4063/4058 respectively.
Complete root wrappers and all mandatory guards pass. Detailed algorithm,
actual service, linked-eZ80 and negative-control checks are recorded in sibling
agon-extender/docs/tasks/PORT-008/DIAGNOSTIC-ROM-RECOVERY-RESULTS.md.

Diagnostic extraction still requires real utility loading and paired peer
qualification when the bench is available. Parallel activation/physical
qualification remain I16-02c gates. No bench/SD/device-network operation,
production selection, commit or push; emulator-coupled changes await review.
Unrelated scripts/application_peer.py remains untouched.

## 2026-10-09 native coordinator partial checkpoint — paused

Author authorized PORT-008 LC02, then requested pause for a network topology
change. Private emos_parallel_coordinator.c now binds the shared boot handover,
reservation, parking/native/unpark and completion exchange. Native-only startup
hooks exclude normal handover from reset monitoring. Faults fence and retain the
reservation until reset. No API/CLI activation caller is installed.

Complete ordinary128739/2333free and private130495/577free compile/link checks
pass; native static RAM4064. Exact sole coordinator/raw-leaf caller inventories
remain guarded. Paired coordinator execution and complete P4 builds remain
pending; no deployment/bench/SD/network-device action, commit or push occurred.
Sibling agon-extender/docs/tasks/PORT-008/LIVE-COORDINATOR-LC02-STATUS.md owns
resume sequence and hashed evidence. Unrelated application_peer.py untouched.

## 2026-10-09 native coordinator software integration complete

Author resumed PORT-008 LC02. The private foreground coordinator, same-owner
boot-monitor exclusion and matched completion binding pass paired host tests
and actual linked-eZ80 checks. Review corrected the final-release failure path
so it also invalidates/fences rather than returning partially closed. The
private coordinator remains unregistered, with zero activation callers; no
ExExt API/mode or automatic file transport is added.

Complete maintained ordinary/private wrappers and unchanged link/ROM guards
pass: ordinary 128739 ROM bytes used /2333 free; private 130473 /599 free,
4064 static RAM bytes. The private coordinator adds 1149 bytes over the prior
native-leaf composition. Both complete P4-PC/browser builds also pass. Paired
native checks: 82 cases; P4 coordinator: 264; actual private startup/runtime:
80; native C/assembly: 124; existing UART/control/ISR regressions, two semantic
mutations and nine ELF controls pass. These are off-bench checks with modeled
physical inputs; no electrical timing or throughput is claimed.

Detailed evidence and portable hashes live in sibling
[Extender results](../../../agon-extender/docs/tasks/PORT-008/LIVE-COORDINATOR-LC02-RESULTS.md).
Pause for Author review before a separate bounded physical activation fixture.
No bench/network-device/SD action, production change, commit or push occurred.
All prior dirt, including unrelated scripts/application_peer.py, is preserved.
The earlier paused checkpoint remains historical evidence.
