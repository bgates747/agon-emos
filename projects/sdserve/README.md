# Mainboard SD foreground service

Provisional PORT-017 / INTEG-013 implementation. Requires the new resident
`ext.sdlink` gateway and matching Extender P4 firmware; current historical EMOS
v0.1.13 does not provide it. Build success is not physical qualification.
Onboard VDP remains stock. The application performs no graphics mode change.

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
RUN . /extender/sdtest
```

Escape returns to MOS, preserving unfinished stages. The network EXIT operation
is accepted only without an active transfer. The server does not launch games,
flash firmware, install callbacks or take over UART. An EMOS-owned mailbox
carries one bounded request; all filesystem work runs in the foreground.

The wire contract and host client belong to agon-extender:
`docs/tasks/PORT-017/PROTOCOL.md` and `scripts/sdcard.py`. The local
`src/sd_wire.h` is a reviewed byte-identical copy of its maintained codec;
keep both copies synchronized. No sibling checkout is needed for compilation.

Files are staged alongside the target as `.p17part`, with immutable `.p17meta`
identity and retained `.p17bak` previous target. FINISH checks sync/close then
independent size/CRC readback. ACTIVATE verifies again after renaming. These
steps permit explicit recovery but do not make FAT rename power-failure atomic.
The host client also reads back the complete stage and compares original bytes.
See the contract for recovery state bits and safe ordering; never manually
discard an ambiguous journal just to make the next BEGIN succeed.

Host checks from the repository root:

```sh
.venv/bin/python -m unittest discover -s tests -p test_sdserve.py -v
.venv/bin/python -m unittest discover -s tests -p test_emos_sdlink.py -v
```

The filesystem adapter tests the real engine using injected POSIX failures;
it is not FAT or hardware proof. Extender's headless runner tests the compiled
eZ80 application, actual EMOS/FatFS, raw FAT image, maintained P4 queue and host
HTTP client. Physical acceptance remains required. The ordinary directory SD
backend has known create-new/sync differences documented in Extender REMED-003.

For an identified candidate from clean committed source:

```sh
.venv/bin/python scripts/prepare_sdserve.py --output build/sdserve-candidate --toolchain /absolute/path/to/agondev
```

The wrapper stamps the banner and preserves a YAML manifest with source commit,
source/compiler hashes and executable digest. Ordinary make keeps an explicit
unversioned development identity and is not a commissioning payload.
