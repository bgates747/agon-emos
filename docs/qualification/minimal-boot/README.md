# Ordinary EMOS boot — draft test sheet

Scope: the `agon-emos-v0.2.0` bundle prepared under
[QUAL-002](../../tasks/QUAL-002.md). This draft is for emulator review, not an
instruction to flash the current dirty build. The Author must review the
emulator, approve freezing the inputs and the procedure's revision identity,
and authorize physical deployment before W3. Preserve completed results in a
`runs/<QUAL-002-UTC-timestamp>/` directory beside this sheet.

The Author accepted the first graphical emulator pass and authorized the
reviewed source checkpoint on 2026-09-07. Its exact image and results are in
QUAL-002; deployment-candidate freeze and physical approval remain pending.

The eZ80 runs EMOS in Legacy. Its UART0 talks to the unchanged onboard VDP;
the onboard VDP supplies PB1 VSync. The ordinary SD program uses `mos_fopen`,
`mos_fread`, `mos_fclose`, `mos_sysvars` and ordinary text output. It verifies
an exact file, observes a change in the low MOS-clock byte, then returns zero.
It does not select an Extender mode or access UART1/Port C.

## Prepare and review on the workstation

Use the project virtual environment, created with `venv --copies`, and install
`requirements-dev.txt`. From the EMOS repository:

```text
.venv/bin/python scripts/prepare_boot_review.py --output build/<new-directory> --fab-root <official-Fab-checkout>
.venv/bin/python scripts/review_boot.py --bundle build/<new-directory> --fab-root <official-Fab-checkout>
cd build/<new-directory>/review/profiles/emos
./fab-agon-emulator
```

The first command uses the ordinary source profile, a new prepared worktree,
the complete configured builder gate, all registered final-image checks, EMOS
tests/module checks and the existing separate EMOS runtime regression. It
produces identified binary/ELF/map/smoke files and `build-manifest.yaml` with
hashes, source/build-tool revisions and dirty-state evidence. A dirty review
bundle cannot qualify hardware. Never select the fixed parallel profile here.

The second command checks the same smoke on stock MOS v3.0.2 and the identified
EMOS image, including deliberately incorrect SD contents. It creates two
isolated graphical profiles. Their generated launchers check firmware, map,
native VDP, emulator and exact test-media hashes before launch. `--check`
verifies a profile without opening a window. Launch each profile from its own
directory through `./fab-agon-emulator`; `review/profiles/stock` provides comparison.

The Author observes automatic execution, visible PASS output and prompt return
without typing. In EMOS, both status reports must show the expected build,
Legacy, onboard VDU and inactive EDU. The headless emulator's synthetic VSync
and directory-backed SD tests do not establish physical UART baud, SD timing,
electrical inactivity or a measured 60 Hz signal. The linked UART0/UART1 guard
must separately report divisor **1** with 32-bit arithmetic in the final ELF.

## Exact smoke media and expected output

Both media trees contain only these test files:

| File | Contents |
| --- | --- |
| `/bin/EMBOOT.BIN` | The manifest's `smoke_program` output, renamed for MOS |
| `/emos-boot/check.txt` | Exactly `EMOS SD CHECK` followed by CRLF, 15 bytes |
| `/autoexec.txt` | The appropriate script below, with CRLF line endings |

Stock MOS script:

```text
LOAD /bin/EMBOOT.BIN
RUN
```

EMOS script:

```text
EMOS STATUS
LOAD /bin/EMBOOT.BIN
RUN
EMOS STATUS
```

The smoke prints its exact bundle build ID and status, then:

```text
BOOT SMOKE SD PASS
BOOT SMOKE CLOCK PASS
BOOT SMOKE PASS - returning to MOS
```

Each EMOS status report must contain:

```text
EMOS identity: agon-emos-v0.2.0, build <exact build ID>, status <frozen status>
EMOS v1: Legacy, registry 0, generation 0
VDU route 0, EDU inactive, adapter unavailable, mode generation 0
```

No mode-setting utility, module discovery, fake adapter or keypress is needed.
A nonzero smoke result stops autoexec before the final status. Any `BOOT SMOKE
FAIL`, boot-script error or unexpected mode is a failure, even if a prompt
appears afterward. Clock progress is required, not a particular increment;
the finite polling limit is a stalled-clock escape, not a calibrated timer.

## Physical preparation and one-shot MOS installation — W3 only

1. After Author approval, freeze committed source, configuration and the test
   procedure. Build from clean inputs with a new UTC build ID. Retain the
   complete manifest, logs and output hashes. Recheck the final binary/ELF pair;
   an earlier build's passing UART check does not authenticate new bytes.
2. Read the current ignored bench record and
   [BC-001](../../../../agon-extender/docs/qualification/bench-constraints.md).
   The operator powers off and isolates the r03 harness from the Agon for this
   EMOS-only proof. Keep onboard VDP/UART0 and PB1 intact. Leave WROOM unwired
   and its firmware untouched. Record the installed MOS/VDP versions visually.
3. With the Agon off and its SD mounted on the workstation, preserve the
   existing boot files and every destination file before replacement; keep
   backup bytes and hashes outside the card. Account for case-insensitive
   names. Disable `/!boot.obey` and `/autoexec.obey`, which take precedence
   over `/autoexec.txt`. Save the pinwalk autoexec rather than allowing it to
   run. The workstation stages the stock smoke first, syncs and unmounts the
   card; the operator proves its automatic stock-MOS pass before installation.
4. Use the unmodified upstream
   [agon-flash v1.9](https://github.com/AgonPlatform/agon-flash/releases/tag/v1.9)
   `flash.bin` as `/mos/flash.bin`: 15,624 bytes, SHA-256
   `480b476703b798cf8d93294cac42d77cad93f7747003b061520b86685c75eb57`.
   Its source is pinned at `e670b5bd910dfe372c29aa9e896e6c24cc8530ec`.
   Confirm the MOS command resolves to this file. The `mos` argument selects
   only eZ80 MOS; `-f` skips the keyboard confirmation. Do not use `batch`,
   which selects both default firmware payloads.
5. On the workstation, require `/EMNEW.BIN` and `/EMDONE.BIN` both to be absent;
   preserve/conflict-resolve existing files before proceeding. Copy the exact
   frozen firmware as `/EMNEW.BIN`, verify its hash after copying, and write
   this two-line CRLF `/autoexec.txt`:

   ```text
   RENAME /EMNEW.BIN /EMDONE.BIN
   FLASH MOS /EMDONE.BIN -f
   ```

6. Sync/unmount before the operator inserts the card and powers on. Observe
   the flash utility's `Checking CRC... OK`, `Done` and automatic reset.
   The utility compares CRC32 read from programmed eZ80 flash with the loaded
   payload. Record the output, not merely the subsequent banner. Allow up to
   two minutes for observation before recording a timeout; do not interrupt
   an active erase/write or retry blindly.
7. On reset, `/EMNEW.BIN` is absent. The first line fails and MOS stops the
   script before invoking FLASH again. That one expected rename error is the
   installation stop state, not a smoke failure. After the Agon has settled,
   the operator powers off; the workstation remounts the card, preserves the
   install script and replaces it with the EMOS smoke script above. Keep the
   renamed payload as evidence. Verify the staged files, sync and unmount.
8. The operator performs three cold boots. On each, allow 30 seconds from
   power-on for the MOS banner and a further 10 seconds for smoke completion
   and final status/prompt. Record missing startup, timeout, reset or wrong
   output as failure and stop further candidate attempts for diagnosis.

The rename guard follows the previously exercised installation pattern retained
in Extender's [PORT-008 record](../../../../agon-extender/docs/tasks/PORT-008.md).
MOS 3.0.2's EXEC stops at the first failed command; renaming before flashing
consumes the one-shot source filename without editing an executing script.
This is a two-stage SD handover, not an automatic return to the smoke script.

## Result record

Record the run ID/start time, input commits/clean state, build ID/status and
hashes, exact stock and EMOS/VDP identities, card preparation/backup record,
harness isolation, flash CRC result, and one row for each of the three boots:

| Boot | Identity correct | SD PASS | CLOCK PASS | Legacy/inactive EDU | Prompt within limit | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | | | | | | |
| 2 | | | | | | |
| 3 | | | | | | |

Retain raw photos/video or available capture, hashes and the Author's acceptance
beside the run manifest. This establishes a bounded ordinary boot baseline;
full MOS compatibility and UART1/P4 functionality remain untested.

If EMOS cannot reach autoexec, an SD recovery file cannot help. Retain stock
MOS v3.0.2, 108,490 bytes, SHA-256
`d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae`.
Only if recovery becomes necessary, present the concrete external WROOM ZDI
operation recorded in [QUAL-002](../../tasks/QUAL-002.md#recovery-payload-and-method)
for authorization. No recovery wiring, firmware preparation or rehearsal is
required in advance. Keep the onboard VDP installed.

## Contract references

1. Official documentation snapshot `f9806bd3cbff6ed5d1c08bef1d51fed11764b86b`:
   `docs/MOS.md` boot-file priority and stop-on-error scripts;
   `docs/mos/Star-Commands.md` LOAD, RUN, RENAME and EXEC;
   `docs/mos/API.md` file handles/read/close and MOS sysvars;
   `docs/Updating-Firmware.md` update and recovery limits.
2. Official MOS v3.0.2 `src/mos.c`, `mos_EXEC` and `mos_cmdRUN`: stop on the
   first nonzero result and propagate the ordinary application's result.
3. Upstream agon-flash v1.9 `src/main.c`, option parsing, `update_mos` and
   reset path: MOS-only `-f`, RAM/payload and programmed-flash CRC checks.
