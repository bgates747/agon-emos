# Stock-MOS smoke preparation — 2026-09-07

State: the Author reports all requested stock-MOS hardware checks passed.
The card is remounted for EMOS installation preparation.
This is preliminary ordinary-application evidence for
[QUAL-002](../../tasks/QUAL-002.md), not a completed qualification run or an
EMOS installation. The Author reported the Agon and P4 powered and connected,
with the SD mounted on the workstation. Operator confirmation of powered-off
harness isolation remains pending.

## Checked inputs

Clean EMOS commit `30e6ea047c657462e57a4c0e76426cd5656f81c3` and clean
mos-agondev commit `b0ed60f39e103a1d697094356aa0b4a2ca1909e5` produced
`agon-emos-v0.2.0-b2026-09-08-00-46-20Z`, still at **draft** status. Both
manifest dirty flags are false. The complete configured gate, all registered
linked checks, the existing EMOS runtime regression, and ordinary stock/EMOS
positive and bad-SD controls pass. UART0/UART1 divisor is 1 in the final ELF.

| Output | Bytes | SHA-256 |
| --- | ---: | --- |
| EMOS firmware, retained on workstation only | 116552 | `e682683604d50c4275f08ec38eef80a3c3dba499f84708ad448fd966acf5900b` |
| ELF | 315260 | `3326ca6d35fe0b33c24aa47d6b83d91938b6b4db75a6d02394b78e1b14d2b7a0` |
| Ordinary smoke application | 1779 | `ba998b7719a23a6ea9a01cd6e08607c2d0b04f9daa417d8af04d2ccdba943f60` |

Local build/check evidence is in ignored `build/boot-clean-01/`. The firmware
is not selected for flash: candidate status and procedure freeze remain
pending. The ordinary smoke runs under installed stock MOS and makes no
Extender API, UART1 or GPIO call. This stock preflight does not qualify EMOS.

## Media preparation

1. Verified the workstation's mounted FAT32 Agon card. Its existing autoexec
   contained `LOAD /bin/PWBOOT.BIN` followed by `RUN`, both CRLF terminated.
   There was no `!boot.obey` or `autoexec.obey` override, and neither new
   installation filename was present.
2. The existing stock `mos.bin` matches the selected MOS v3.0.2 release hash;
   `mos/flash.bin` matches the pinned official agon-flash v1.9 hash in the test
   sheet. A historical `emos-installed.bin` was preserved and is not selected.
   Card payload identity does not prove the currently installed firmware.
3. Preserved original boot/firmware files, absence records and hashes outside
   the card under ignored `build/media-backups/2026-09-08-00-46-21Z/`.
4. Staged the manifest's ordinary smoke as `/bin/EMBOOT.BIN`, the exact
   15-byte `/emos-boot/check.txt`, and this CRLF `/autoexec.txt`:

   ```text
   LOAD /bin/EMBOOT.BIN
   RUN
   ```

5. Verified staged file hashes, synced and successfully unmounted the card.
   `build/boot-clean-01/stock-media-staging.json` records the exact operation.
   No FLASH command or `/EMNEW.BIN` was staged, and neither processor was
   flashed, reset or power-controlled by the agent.

## Pending physical observation

Follow the [test sheet](README.md): the operator powers off both boards,
disconnects the r03 ribbon at the Agon GPIO header, inserts the prepared card
and cold-boots the Agon using its unchanged onboard VDP. No keyboard input is
needed. Record the displayed MOS/VDP versions, exact smoke build identity,
SD PASS, CLOCK PASS, final smoke PASS and prompt return. Allow 30 seconds for
MOS startup and a further 10 seconds for the smoke. No physical pass is claimed
until that observation arrives.

## Subsequent mode-selection instruction

The Author directed that video mode changes occur only in autoexec, using
`VDU 22 n`, and selected mode 3 for this smoke. Future staging prepends
`VDU 22 3` as recorded in the updated test sheet and media generator. The
already staged and unmounted card described above contains only LOAD/RUN;
its recorded contents and evidence have not been retroactively changed, and
they do not establish which video mode the VDP selected at boot. Apply the
mode command when the card is next staged. EMBOOT already makes no mode change.

## Mode 3 applied after remount — 2026-09-08 01:01:54 UTC

The Author remounted the card and authorized the update. Verified the same
Agon FAT32 mount, unchanged smoke/fixture bytes and absence of overriding boot
scripts. Preserved the prior autoexec and prepended the mode command. The
current 37-byte CRLF script is:

```text
VDU 22 3
LOAD /bin/EMBOOT.BIN
RUN
```

SHA-256: `188e2dda4a0cb5fc900797cf74d1340806211f807c22c7a4bf5c815829d3f641`.
The backup and operation record are retained in ignored
`build/media-backups/2026-09-08-01-01-54Z/`. Verified the updated contents,
synced and successfully unmounted. Only autoexec changed on the card; the
smoke binary and original build/review evidence remain unchanged. No flash
command was added and physical observation remains pending.

## Author-reported hardware result — 2026-09-07

The Author reported “everything passed on hardware” and remounted the card.
Record the requested ordinary stock-MOS smoke as **PASS**: automatic program
execution, SD contents, clock progress, visible final PASS and prompt return.
The remounted card still contains the exact mode-3 autoexec and smoke hashes
recorded above. This is operator-reported evidence; no new screenshot, physical
firmware-version transcript or timing measurement accompanied the report.
It satisfies the preliminary stock-smoke gate, not EMOS physical qualification.
