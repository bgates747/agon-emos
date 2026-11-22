# INTEG-019 — Linked EMOS build result

The Olimex EMOS candidate adds exactly the same flash and static SRAM as the
standalone NMI API R01 donor. There are **496 bytes free** in the 128 KiB
EMOS ROM. The build is ready for downstream host packet verification.
Physical qualification and the production handback are pending.

## Linked comparison

Subtract the unchanged source control from its candidate within each
profile. The donor uses standalone Eirene RC3. The product comparison
reproduces production Eirene r06 with RFC25 UART0 and both PARLIO SD
directions. The linker maps account for each changed byte.

| Measurement, bytes | Standalone Eirene RC3 | Standalone R01 | Production EMOS r06 | EMOS R01 candidate |
| --- | ---: | ---: | ---: | ---: |
| Flash used | 100,879 | 101,533 | 129,922 | 130,576 |
| Flash added | — | 654 | — | 654 |
| Initialized SRAM | 782 | 782 | 802 | 802 |
| Zero-initialized SRAM | 2,098 | 2,143 | 3,490 | 3,535 |
| Static SRAM total | 2,880 | 2,925 | 4,292 | 4,337 |
| Static SRAM added | — | 45 | — | 45 |
| EMOS heap after its 2,048-byte long stack reservation | — | — | 10,044 | 9,999 |

The 654-byte EMOS delta contains 523 bytes in the imported R01 assembly,
83 bytes for the execution guard and lease ownership context, 35 for the
boot banner, nine for the early watchdog guard, and four for the low-ROM
syscall trampoline. The discovery marker occupies an existing gap in the
ROM descriptor area. The 45 static SRAM bytes are the 33-byte parked
guest frame and 12 bytes of bookkeeping.

The donor's NMI entry holds up to 60 bytes of additional simultaneous
frames on the long stack. Its protected 2,048-byte stack reservation and
the separate guest short stack carry over unchanged. The 60-byte figure
excludes ordinary command, syscall, callback and IRQ working frames and
is not a measured high-water mark. The linked `_mos_runBin` reserves eight
local stack bytes against seven in production, for one more byte at that
point in the call chain. Linked BSS ends at `0x0BD0F1`, below
the heap and long stack. The reference fixture uses guest bank `05h`,
outside that resident SRAM. EMOS startup retains bank `05h` when the
warm-boot magic survives and clears RAM on its cold-boot path. The physical
postboot check must prove the actual watchdog result.

## Identities and verification

- Reviewed and approved [contract](../INTEG-019.md), frozen in
  `8b75388c34609738f1fdcfba53ab5dc3ee668893`. Its accepted SHA-256 is
  `7d234b45e58b8d4ef4d570f9900a0a3df1b906b9c53e37eb268a078e7e8f7478`.
- Production control `abefa2f943eb3089c08d85bf39e48ebb4938f4bd`. The native
  build reproduced the exact approved production ROM, including padding
  to 131,072 bytes. SHA-256
  `9b90b5d94ec6888d85ef1fa1acf23a82b2e0eb1f179b75f549863fbd72343673`.
- Candidate source `272851ed47f44863414c7235271f3605e556fc81` on the isolated
  integration branch. Build `agon-emos-eirene-r06-b2026-11-22-21-30-04Z`.
  Boot variant `Eirene UART0 r06 NMI API R01 [dd2ecb8762e98647]`.
  Full 131,072-byte ROM SHA-256
  `a2544d5e158c19f3bdc5b3a69f28afe2472b8c1c5e6235939629953c965b39de`.
- Both native builds passed the wrapper's mandatory linked startup,
  UART baud and ownership, keyboard, console, and PARLIO checks. Additional
  structural checks passed for marker and syscall discovery, table slot,
  jump targets, QUERY reply bytes, no-owner NMI forwarding, early watchdog
  hook, and the contiguous parked frame. The imported assembly body
  matches the pinned R01 donor byte for byte. These checks do not establish
  physical watchdog, guest-state, keyboard or PARLIO behavior.
- Retained the exact 955-byte R01 fixture and reviewed the linked warm-boot
  path. Its SHA-256 is
  `11e7686e29cdfb864e3ae58e24571a3dd00cc5ab5ced26681a4b00d46b42b216`.
  Keep the physical pass conditional on the postboot watchdog status and
  the fixture's state witness. The preparation uses the same external app
  before and after the candidate reboot.
- Build and linked-check receipts and the private packet are retained in
  the task-local Extender operator record `agents/integ-019`. The reviewed
  contract bytes remain intact. M304 remains paused for physical
  qualification of the exact candidate.

## Recovery and bench preparation

The temporary P4 programmer is a separate recovery build with the exact
production EMOS ROM embedded. The project virtual environment's PlatformIO
6.1.18 could not build the pinned ESP32 platform. The job uses an isolated
PlatformIO 6.1.19 environment, matching the retained successful recovery build.
The current source-selection hook expects an explicit SDK configuration
path, so the job binds a copy of the maintained recovery target's
`sdkconfig.defaults` in its private worktree. The recovery implementation and
the production board configuration are unchanged.

The installed bench runner passed host-only readiness for its current
commissioned revision and the EMOS-only stage/install/restore/handback
manifest. The key observer is a job-local fixture using public MOS system
variables. Candidate before/after checkpoints require physical A and Enter
make/break, a physical DIR, and an independently checked 513-byte PARLIO
round trip. TRS-80 coordinates Smith's attendance, physical input cues,
and monitor observations while Extender owns the bench.

The detached Pi build completed with all compiler workers joined. Host checks
matched the generated P4 partition table and app0 offset to production and
verified the embedded ROM. The recovery app is 464,928 bytes with SHA-256
`744aadf3aa66c536cb6d42402d458129f03a5a1e8c040b82fb51ee204abdb742`.
Its 1,769,472-byte app0 write image has SHA-256
`bca799c326a5989f6c73c91868260476aa3b55cea071b12d1a87fb2b19362464`.
The host-only recovery packet pins the image, source, configuration and
exact production restore. Its SHA-256 is
`58f12701cf0ada5d177c34c8c6f2cb92994fa230a3eaa50cd7dfff0f47d61ec0`.
Physical ZDI dump and production P4 restore are required before candidate
installation.
