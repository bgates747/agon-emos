# INTEG-019 — Qualify NMI API R01 on Olimex EMOS

Integrate the tested watchdog API into the exact production Olimex EMOS
build. Demonstrate that a Z80 guest can run with interrupts disabled, yield
repeatedly to the eZ80 host without losing state, and reboot safely while the
watchdog is armed. The bench must retain physical Extender keyboard input and
two-way PARLIO SD transfers. M304 stays on hold until the resulting candidate
qualifies. Production EMOS is restored before check-in.

**R02 draft, negotiation round 2 of at most 5.** Extender owns the EMOS
integration, generic qualification fixture, Olimex packet and handback. TRS-80
reviews the technical contract and owns the separate M304 BASIC timing fixture.
TRS-80 and then Smith must approve the same exact draft revision or SHA-256.
Stop for those approvals. If five rounds do not settle the contract, report
the unresolved points to Smith and stop. Once approved, freeze and publish
the reviewed contract before implementation. Production promotion and the
M304 timing run are outside this qualification.

## Frozen inputs

Use the [Extender production selector](../../../agon-extender/production/current.yaml),
[v0.3.0 bundle manifest](../../../agon-extender/production/bundles/olimex-parlio-v0.3.0/bundle.yaml)
and [Olimex bench guide](../../../agon-extender/docs/operations/olimex-bench/README.md).
The manifest SHA-256 is
`5db98350a82c440b28ab260475cb4274c46f5840b7561648679544fd456e0942`.
Bind the production companion versions selected by the bench guide. If the
production selection changes before execution, reconcile the pins with both
reviewers before using a different baseline.

| Component | Identity and use | SHA-256 |
| --- | --- | --- |
| Olimex eZ80 EMOS Eirene r06 | `agon-emos-eirene-r06-b2026-11-18-02-44-46Z`, source `abefa2f943eb3089c08d85bf39e48ebb4938f4bd`. Exact baseline and full 131,072-byte restore at `runtime/emos/ROM.bin` in the selected bundle. | `9b90b5d94ec6888d85ef1fa1acf23a82b2e0eb1f179b75f549863fbd72343673` |
| Extender ESP32-P4 | Production `port-008-upload-r05-b2026-11-18-03-43-16Z`, source `be1d94329e55cc5e4243adbcdad73e3c530f9053`, raw app. Installed during all candidate checks. | `539dc7176f079bc6c298c48352b7e94c54db654fb6fdbb9c42897370fb19834b` |
| P4 production restore extent | `runtime/p4/app0-write.bin` at `0x20000`, exactly `0x1B0000` (1,769,472) bytes. Independently read back and compare the entire extent after any temporary P4 firmware. | `10d4b60a67888c2363e79ad29adbfd68d58a8638fd9013af413973655cb33ae4` |
| P4 production partition table | `runtime/p4/partition-table.bin`, layout verification only. | `e29396a4f5ecc129c0e275d2df19d69adb5ee33389e3d5659e9a50932ac6864a` |
| Mainboard VDPlet | Selected `v0.1.3-alpha` app in `vdplet-v0.1.3-alpha-async-r1`. Retain VDPlet firmware and flash layout. | `90edc4c52f3120c1709e3f09dd675492e8326f64b36061570b3ea4f2a0efeccc` |
| Foreground SD listener | Production `sdserve-v0.4.0-b2026-11-18-02-44-55Z`, source `b949b7505f121b58623a94b31bb9b53a75b7819f`. Retain `/emos/sdserve.bin`. | `98492acc7dcd29e484ef76c69026e8801375738c33c617ba61a94d8481151f08` |

Start from the verified EMOS source and profile named by production. Use the
bundle's build receipt, source archive, linked map and restore ROM. Keep the
[official MOS ABI](../../../../agon-docs/docs/mos/API.md) and
[keyboard contract](../../../../agon-docs/docs/mos/Keyboard.md) as read-only
references. Treat the [R01 fork API](../../../agon-mos/docs/tasks/NMI-001/API-R01.md)
as the interface under integration. Its
[qualification record](../../../agon-mos/docs/tasks/NMI-001/API-RESULT-R01.md)
covers Flexe and Console8. The Console8 report attests to the yields and boot
by observation and the retained artifact hash covers the staged binary.

The donor is the RC3 source export at `97311c437093e416342a6c836cf9914f2ced027d`
plus the R01 overlay in the Author-owned `agon-mos/experiments/nmi001-api`
checkout. Use these content pins for the donor files and freeze a copy with
the candidate package. The donor binaries are reference artifacts.

| R01 reference | SHA-256 |
| --- | --- |
| `docs/tasks/NMI-001/API-R01.md` | `144a427fe5009e5dc4d3915d53caebae06abf21d6df3026d200856997d83ecfa` |
| `experiments/nmi001-api/nmi_api.asm` | `e000cf0b53017ce49496bd721e9c3d5e1e17154702bdb1c7b5cca94f4f1aba3d` |
| `experiments/nmi001-api/prepare.py` | `9dc605613922decf5ea677ad4333a5c6238a5eb98314f6795cbd86c60335311c` |
| `experiments/nmi001-proof/prepare.py` (`replace_once` dependency) | `348d2a990c32853a88a8d406ae020f82f71df48caffc4aa7552465c7f8aa35d0` |
| `experiments/nmi001-api/fixture.asm` | `6a95ce75e5acfa351cacbcca5b034d43f4cfaa7f348599a1ef907c65898c31ef` |
| Console8 `mos-nmiapi.bin`, 101,533 bytes | `9b305f73d077807813257cdb7a64583b7df8fc5e95717f5cd4c464e00d52c10a` |
| External `nmiapi.bin`, 955 bytes | `11e7686e29cdfb864e3ae58e24571a3dd00cc5ab5ced26681a4b00d46b42b216` |

## Scope and acceptance checks

- INTEG-019-001 [ ] Obtain TRS-80 technical approval and Smith's final
  approval of the same exact contract, including Smith's explicit approval
  of the temporary P4 firmware exception below. Freeze the contract before
  source changes, builds or reservation.

- INTEG-019-002 [ ] Integrate R01 into the pinned EMOS source through its
  maintained build wrapper with all required link checks. Keep ordinary
  EMOS IRQ/NMI, Extender keyboard, VDU routing and PARLIO ownership intact.
  Check the actual production vector, ROM marker range, table slot, early boot
  and application-return hooks before porting the donor. Give the candidate a
  human-readable boot banner retaining the Eirene r06 identity and adding
  `NMI API R01` plus a candidate-specific build identifier. Bind that exact
  banner to one compiled image in the packet. An occupied marker or slot, or
  a change to the public ABI or production transport, requires a reviewed
  contract amendment.

- INTEG-019-003 [ ] Verify linked API, ROM and SRAM offline. A caller checks
  `NM1` followed by `63h` at ROM `0000E0h` before calling `RST.LIL 08h`
  with `A=63h` from the outermost ADL application at `040000h`, `MB=0`, IRQs
  enabled. QUERY must return `A=0`, `DE=314D4Eh`, `HL=070101h`. Verify
  read-only QUERY, operations 0–4, documented errors, register preservation,
  ownership, unarmed return cleanup, no-owner NMI behavior, and early boot
  and armed exit. Use unmodified production as the unsupported-discovery
  control and confirm the caller never dispatches the absent syscall. Report
  linked ROM usage and free bytes in the 131,072-byte extent, initialized
  SRAM, BSS, reserved regions and remaining RAM. Account separately for the
  33-byte parked frame, ownership state, protected long-stack depth including
  normal callers/IRQ/callback work, and the separate guest short stack and
  banks. Require positive ROM headroom and a justified positive SRAM and
  stack margin with no overlap. Show baseline versus candidate from the
  preserved baseline and candidate maps and distinguish any build-only
  differences. Include linked checks for the ordinary keyboard, startup and
  production PARLIO paths. Emulator evidence, if used, follows its distinct
  review and reservation requirements.

- INTEG-019-004 [ ] Commission one bounded EMOS qualification packet and its
  P4 recovery phases before requesting the slot. Pin source revisions and
  deltas, wrapper and toolchain, candidate
  build ID and full-ROM hash, ELF/map and link evidence, fixture hash, exact
  commands and expected witnesses, runner/config identities, safe startup,
  restore bytes and keyboard-independent recovery packet. Prefer the exact
  R01 external fixture where the linked EMOS map and startup preserve its
  retained RAM witness. Otherwise pin and review a minimal generic
  adaptation before the run. Preserve all guest-state assertions, collect a
  postboot watchdog observation independent of the saved guest witness, and
  use no private EMOS RAM addresses. Follow the
  [bench authoring guide](../../../agon-extender/docs/operations/olimex-bench/AUTHORING.md)
  and [test-appliance lifecycle](../../../agon-extender/docs/procedures/test-appliances.md).
  Preserve `/autoexec.txt`,
  stage only in the job's `/test` directory, and confirm startup before any
  operation that reboots. Set one 60-second host deadline for the armed
  fixture cycle from dispatch through a new CLI, with the reset witness
  collected continuously. Commission finite service and recovery deadlines
  separately using their maintained tools. Extender arranges a named person
  physically at the Olimex to operate the production P4 USB keyboard. Before
  checkout, confirm their availability and the windows for the production
  preflight, both candidate boots and production handback. Smith may do this
  if he explicitly agrees to the windows. Defer checkout until Extender has
  that confirmation. Require the independent recovery preflight below before
  the candidate write.

- INTEG-019-005 [ ] Complete the P4 ZDI read-only EMOS preflight, independently
  verify the restored production P4 and prove its keyboard and PARLIO
  services. Then run the unsupported control from the baseline MOS prompt
  and install the candidate EMOS from the healthy production CLI
  via the commissioned `install-emos` route. Compare all 131,072 bytes of
  independent readback to the candidate, retain the receipt, and observe the
  exact candidate banner and Legacy CLI. Before running the armed case,
  demonstrate P4 keyboard and PARLIO SD behavior as defined below. Run the
  unarmed API checks and prove ordinary CLI return and a clean reacquire.

- INTEG-019-006 [ ] Issue one explicit CLI invocation of the armed case and
  record eight watchdog callbacks and seven resumes from managed native Z80
  code running with `DI`, using R01's SCLK `2^22` cycle watchdog profile and
  explicit reload rule. Per yield, prove a watchdog cause, correct callback
  context and guest PC, advancing guest state, preserved primary and alternate
  registers including applicable upper bytes, indexes, flags, bank, SPS and
  short-stack guards. Verify the guest resumes with IRQs disabled and the
  ADL foreground callback services the host with IRQs enabled and actual MOS
  VBlank progress. Check that the callback and host work fit the same
  watchdog interval. Keep end-to-end PARLIO testing outside this armed window.
  Require successful armed `RELEASE` to take the documented watchdog reset
  path with no successful return. Capture fresh candidate boot and CLI
  without an operator reset. On boot, verify the watchdog reset cause and
  enable bit clear (`WDT_CTL=20h`, observed by the fixture before guest RAM
  reuse), then verify a fresh acquire and unarmed release. Report any absent
  witness or unexpected reset as a failed case.

- INTEG-019-007 [ ] Repeat the keyboard and PARLIO SD witnesses on the fresh
  candidate boot. At each checkpoint use a physical keyboard connected to
  the production P4 USB host. Record press and release for a printable key
  and Enter through the P4-to-EMOS keyboard path, using the documented public
  keyboard observation path and fresh admission after reboot. Execute an
  actual CLI command such as `DIR` via that keyboard and capture its result
  and prompt. For SD, the Pi sends a 513-byte deterministic test payload
  through the production P4 to EMOS over PARLIO and the stock foreground
  listener writes the card. Retrieve the file in the reverse direction over
  PARLIO and compare all 513 bytes on the Pi. Use a fresh scratch name and
  payload for each checkpoint, record the selected transport and reject an
  automatic UART fallback. This one test comparison is an acceptance oracle.
  The production transport and routine fixture staging retain their
  checksum-free policy. Exit the listener and confirm the Legacy CLI.

- INTEG-019-008 [ ] After a successful run with a healthy CLI, restore and
  independently verify the full production ROM with the commissioned
  `restore-emos` phase. Use the preflighted recovery path below if the
  ordinary route is unavailable or fails, subject to Smith's disposition.
  Then verify the working restored CLI,
  physical P4 input, production SD operation, idle listener/transfers, closed
  serial handles and joined workers. Preserve the original failure evidence
  if any. Send ordinary scheduler check-in only after the bench guide's
  handback criteria are met.

- INTEG-019-009 [ ] Deliver the exact qualified candidate and handoff record
  to Smith and TRS-80 after all qualification and handback checks pass. List
  source and donor pins, final binary ID/size/hash/banner, production restore
  pin, packet and fixture hashes, physical per-case receipts and known limits.
  M304 may use this result as a prerequisite for a separate approved run.
  The timing fixture and its invocation remain with TRS-80.

## EMOS restoration and failure boundary

The [Olimex bench guide](../../../agon-extender/docs/operations/olimex-bench/README.md)
governs custody, commissioned operations, failure handling and handback. Its
ordinary `restore-emos` uses `FLASH mos` followed by independent readback
of the full ROM through the MOS CLI and foreground listener. Use that
phase after a successful run with a healthy CLI. If a failed test leaves
the ordinary restore available, use the guide's failed-phase recovery
procedure once Smith authorizes recovery.

**Exception proposed for Smith's explicit approval.** Extender will use the
maintained [P4 ZDI procedure](../../../agon-extender/docs/mos-recovery.md)
as the sole fallback when ordinary EMOS restore is unavailable or fails.
Temporarily install recovery-only firmware on the Extender P4 for the
preflight and, only if needed after Smith's failure disposition, for one
recovery operation. Production P4 runs during all candidate checks. Keep
mainboard VDPlet installed throughout. Reuse the existing P4 recovery
implementation. WROOM work is outside this task's scope.

Before checkout, the Extender operator binds the reviewed recovery source
revision, `p4-mos-recovery` profile, build configuration, embedded exact
production EMOS ROM, documented upstream flash-agent identity, resulting
P4 image and its flash extent and SHA-256, and maintained host console and
supervision versions. Bind both the approved scope and the job evidence
directory. Verify the recovery image fits within the pinned production P4
restore extent before scheduling. Verify P4 identity and partition layout
against the job binding before writing. Limit the temporary change to
P4 app0 and preserve bootloader, partition table and other flash contents.
Bind the actual ZDI harness and target identities in the local job binding.

For preflight, follow the documented powered-off wiring procedure if leads
need fitting, then use the identified P4 as the ZDI programmer. Obtain the
documented consistent ZDI product-ID reads and a complete 131,072-byte
dump in dump-only mode. Independently compare it with the exact production
EMOS ROM. The preflight performs no eZ80 erase or write. Reinstall the
pinned P4 production app0 extent, independently verify the full readback,
then boot Agon and establish a working production MOS CLI with fresh
keyboard admission. Prove actual P4 keyboard input and production PARLIO
SD service. Candidate installation requires all preflight, P4 restoration
and service checks to pass.

If the fallback is authorized, rebind the same preflighted programmer and
use the maintained verified pre-erase dump gate and explicit restore arm.
Independently compare the complete 131,072-byte production EMOS write over
ZDI. Restore the exact production P4 extent and independently compare its
full readback before resetting Agon. Use the already confirmed safe startup
and establish fresh keyboard admission, actual CLI and SD service, then
complete ordinary handback. ZDI provides restoration and verification with
candidate keyboard, CLI and SD listener unavailable.

On the first unexpected result, stop target operations, retain raw capture,
operation timestamps, hashes, worker terminal/join receipts and the last
verified bench state. Keep the reservation, report in chat and use the
Oryx spoken alert for immediate attention. Follow Smith's disposition and
the bench guide for recovery. No automatic replay or reset of a failed
fixture. Document a failed or uncertain write before any recovery and keep
custody until production is independently verified and handback completes.
Evidence includes passive boot and CLI capture, R01 per-yield assertion
results, physical key events, the two PARLIO direction receipts and host
comparison per boot, install/restore full-ROM readbacks, and check-in receipt.

## Decisions for round 2 review

| ID | State | Decision |
| --- | --- | --- |
| INTEG-019-D01 | Revised for TRS-80 review and Smith's explicit approval | Use the maintained P4 ZDI route, with the temporary recovery-only P4 firmware exception limited to read-only preflight and authorized fallback. Use the ordinary EMOS restore when available. |
| INTEG-019-D02 | TRS-80 accepted in round 2, pending final review | Use the exact pinned fixture if the linked map and startup preserve its witness. Otherwise pin and review a minimal adaptation that retains the assertions and observes watchdog state independently of the saved witness, without private EMOS RAM addresses. |

**R02 disposition.** Contract draft only. No implementation, build, flash or
bench reservation has been started for this task.
