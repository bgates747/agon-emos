# Mainboard SD foreground service

The service uses the resident `ext.sdlink` gateway and matching Extender P4
firmware. EMOS v0.1.14, sdserve v0.1.0 and uart-excom-console r12 passed scoped
physical file-transfer and native-keyboard checks on 2026-09-13 UTC; see the
[dated acceptance record](../../research/devlog/2026-09-13.md). The frozen
artifact identities remain candidates, not a general release. EMOS v0.1.13
does not provide this gateway. No custom onboard VDP is required and the
application performs no graphics mode change.

Exploratory build with AgonDev on PATH:

```sh
make -C projects/sdserve
```

Or supply `AGONDEV_TOOLCHAIN=/absolute/path/to/agondev`. Output is
`projects/sdserve/bin/sdserve.bin`. Keep that filename on the card: it is a
reserved write target so this service cannot replace its own executing file.
The only application argument is its absolute allowed filesystem root; default
`/extender/sdtest`. Provision that directory first. Typical MOS startup:

```text
VDU 22 3
SET KEYBOARD 1
EMOS KEYINPUT extender
LOAD /extender/sdserve.bin
RUN . /
```

The explicit `/` permits whole-card development and is now used by the bench
startup; `/extender/sdtest` was the initial commissioning scope. Changing the
root requires stopping and restarting the foreground service.

Escape returns to MOS, preserving unfinished stages. The network EXIT operation
is accepted only without an active transfer. The server does not launch games,
flash firmware, install callbacks or take over UART. An EMOS-owned mailbox
carries one bounded request; all filesystem work runs in the foreground.

The wire contract and host client belong to agon-extender:
`docs/protocols/mainboard-sd.md`, `docs/mainboard-sd.md` and `scripts/sdcard.py`.
Historical task paths forward to these maintained authorities. The local
`src/sd_wire.h` is a reviewed byte-identical copy of its maintained codec;
keep both copies synchronized. No sibling checkout is needed for compilation.

Files are staged alongside the target as `.p17part`, with immutable `.p17meta`
identity and retained `.p17bak` previous target. FINISH checks sync/close then
independent size/CRC readback. ACTIVATE verifies again after renaming. These
steps permit explicit recovery but do not make FAT rename power-failure atomic.
The host client also reads back the complete stage and compares original bytes.
See the contract for recovery state bits and safe ordering; never manually
discard an ambiguous journal just to make the next BEGIN succeed.

Do not replace a file held open by another owner. MOS keeps an EXEC/OBEY batch
open while its child application runs; exit the service and restart it directly
at the CLI before replacing that batch. This FatFS build has `FF_FS_LOCK=0`:
the service is cooperative foreground file access, not a global file-lock
monitor. A hung eZ80 cannot serve requests, and this interface supplies neither
automatic hardware reset nor remote game execution.

Host checks from the repository root:

```sh
.venv/bin/python -m unittest discover -s tests -p test_sdserve.py -v
.venv/bin/python -m unittest discover -s tests -p test_emos_sdlink.py -v
```

The filesystem adapter tests the real engine using injected POSIX failures;
it is not FAT or hardware proof. Extender's headless runner tests the compiled
eZ80 application, actual EMOS/FatFS, raw FAT image, maintained P4 queue and host
HTTP client. New candidates require fresh physical validation; the historical
pass does not validate a rebuild. The ordinary directory SD
backend has known create-new/sync differences documented in Extender REMED-003.

For an identified candidate from clean committed source:

```sh
.venv/bin/python scripts/prepare_sdserve.py --output build/sdserve-candidate --toolchain /absolute/path/to/agondev
```

The wrapper stamps the banner and preserves a YAML manifest with source commit,
source/compiler hashes and executable digest. Ordinary make keeps an explicit
unversioned development identity and is not a commissioning payload.
