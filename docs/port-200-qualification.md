# PORT-200 qualification record

This document freezes the locally executable qualification matrix for the
AgonDev EMOS candidate. It distinguishes observed behavior from structural
evidence and from work that requires real hardware. `make verify` is the local
gate; `docs/port-203-hardware.md` is the separate physical gate.

## Emulator parity matrix (PORT-201)

| Area | Candidate/reference observation | Automated gate | Boundary |
|---|---|---|---|
| Boot and shell | Both boot, mount isolated hostfs, and agree on command parsing, HELP (apart from EMOS), variables, errors, RTC display, and exact nested file bytes | `firmware-parity-check` | Fake VDP text output only |
| Keyboard/editor | Every scripted command must echo in order, including spaces, navigation, failures, and return to `/` | `firmware-parity-check` | Stdin-to-keyboard bridge; no physical key matrix or graphical cursor claim |
| VDP startup | Linked candidate initializes and polls GP before the banner | `vdp-regression-check` | Structural linked-byte proof |
| VDP framing | Linked candidate executes idle noise, corrupt-state recovery, legal lengths 0–16, partial bodies, back-to-back GP recovery, and discard lengths 17–255 with a 128-instruction per-byte ceiling | `vdp-regression-check` | Instruction budget, not cycle/electrical timing; stock v3.0.2 is retained only as the known-bad oversized-packet negative control |
| Read-only FatFS | Candidate and stock MOS agree on open/read/seek/tell/EOF/stat/directory/find/path/label operations, including a sparse offset above 24 bits | `contract-check` | Directory-backed hostfs, not an SD controller |
| Writable FatFS | In separate disposable trees each image performs MOS-handle and save/load/copy/rename/delete operations plus direct FatFS create, directory/chdir, write/put, close, rename, stat, cold-boot readback, unlink, and directory removal; exact snapshots prove cleanup | `contract-check` | Fab 1.2.3 does not trap `_f_sync`; its `_f_truncate` path rebooted the guest, and its direct `f_putc` trap does not preserve MOS's BC byte-count return, so only the written byte is asserted |
| RTC | Candidate and reference agree on formatted RTC reads; linked packet receive framing is covered | `firmware-parity-check`, `vdp-regression-check` | Fab CLI implements RTC read mode 0 but not set mode 1, so mutation/persistence is a physical/full-VDP cell |
| UART | Boot and protocol traffic traverse the linked UART path; fixed raw UART PUTCH bytes remain identical to official MOS | `emos-vdu-check`, `test_emos_vdu` | Fab models a 16-byte TX FIFO and an explicitly arbitrary 4x RX slowdown, with no physical line-error source; error flags and timing remain hardware cells |
| EMOS | Discovery, star/service calls, fake Dual transition, and Legacy recovery execute in target MOS | `emos-runtime-check` | Fake adapter is not physical EDP discovery |

Every emulator process has a positive timeout and a one-MiB output ceiling.
Writable runs use newly created temporary directories, exact before/after
snapshots, fixed payload bytes, two separate emulator processes per firmware,
and cleanup verification. Output normalization removes only declared VDP
transport noise and line-ending differences; raw failures remain visible.

## Public ABI inventory and coverage (PORT-202)

The authoritative maintained `mos_api.asm` contains 127 high-level slots (60
currently non-placeholder entries), 38 FatFS slots, and a 33-entry ADL C
function table (17 nonzero entries, including EMOS at `0x20`). The pinned
AgonDev header declares 128 public externs and the source audit inventories all
123 libmos wrapper sources. The target contract currently makes 74 named
assertions; the wrapper audit also checks compiler call slots, archive member
selection, and all 126 IX operands.

The executable matrix is classified as follows:

- Pure and path/string calls: pattern comparison, argument/string/number
  extraction, escaping, GS init/read/translate, substitution, absolute and
  directory path resolution, leaf lookup, and missing-directory errors.
- Return-width and pointer calls: signed `int8_t`, zero-extended `uint8_t`,
  `uint24_t` high-byte arguments and returns, 32-bit seek/tell/size values,
  system time, file-object pointers, and pointer-to-pointer outputs.
- Read and directory calls: both handle and FIL APIs, stat/enumeration/find,
  label/cwd, EOF/error, sparse seek/read, and exact content.
- Write and state calls: both handle and FIL writes plus directory, chdir,
  rename, cold-boot persistence, unlink, and cleanup.
- Callbacks and process-local state: EMOS gateway/dispatch and mode transaction
  callbacks are executable; keyboard/vector callbacks are structurally
  checked but not installed by the unattended probe because a bad vector can
  strand the machine.
- Hardware/destructive/unsupported: raw SD init/read/write, I2C, UART1 external
  traffic, interrupt/vector mutation, RTC set, VDP flag timing, mount/format,
  volume-label mutation, `f_sync`, and `f_truncate` remain explicit unrun or
  emulator-unsupported cells. They are not silently included in the 74.

`mos_extractnumber` is kept in a noinline small-frame reproducer and passes on
both images. The earlier candidate-only failure occurred only when that same
call was embedded in the former monolithic large probe frame. Splitting the
call is the minimized safe workaround. The wrapper has aligned three-byte
slots and the source-wide audit finds no matching wrapper defect, so the
observation remains a compiler/probe frame-sensitivity limitation rather than
a MOS defect claim.

## Upstream pin decisions (PORT-204)

Public heads were checked before this qualification. AgonDev public `master`
still equals `b67ab2444a63267a42193f204889d466765d8dd2`, and Fab public `main`
and tag 1.2.3 equal `fbb7d7ca887a06966ca8a62ed22272409a5ab640`.
The three libmos defect shapes and Fab's 24-bit object-size write remain at
those pins. Therefore no upgrade is justified and the three smallest local
wrapper replacements remain selected. Their failing archive shapes, corrected
disassembly, runtime passing cases, and the bounded Fab workaround are
reproduced by `contract-check` and documented in `docs/upstream-findings.md`.

## Public-input policy (PORT-205)

The maintained source is publicly reachable from
`https://github.com/bgates747/agon-mos.git` at
`a8dc891a3668660978fa66d54b3d7638ecc876e2`; its parent
`8336409351ee5314e02801a7b72a4f1bb5282519` is the official v3.0.2 source.
AgonDev, Fab, and Agon documentation are pinned by public commits in
`evidence/baseline.json`. The stock MOS/map and VDP module are accepted only at
their recorded sizes and SHA-256 hashes.

Baseline schema 2 deliberately excludes a locally compiled Fab executable:
compiler and path differences can change it without changing the pinned source
or firmware assets. `scripts/verify_emulator.py` still hashes the exact
executable used in each configured profile. `audit_source.py` ignores
untracked build products and submodule worktree dirt while rejecting tracked
superproject changes. Thus `baseline-check` is reproducible from documented
public Git inputs and still fails closed on source, toolchain, or stock
firmware drift.
