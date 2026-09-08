# EMOS module tooling

`emos_module.py` is the canonical EMOS v1 container generator and validator.
It consumes a JSON manifest plus a raw provider payload linked for `0x0B0080`,
emits one deterministic `.emo` image, and can inspect or validate images.

```bash
.venv/bin/python projects/emos/emos_module.py build manifest.json payload.bin provider.emo
.venv/bin/python projects/emos/emos_module.py inspect provider.emo
.venv/bin/python projects/emos/emos_module.py validate-dir path/to/modules
```

The first 128 bytes are the Core-owned header. All multibyte integers are
little-endian. The payload begins at offset `0x80`; its link address is therefore
`0x0B0080`. CRC-32 uses the standard reflected polynomial and the same result as
Python's `zlib.crc32`.

| Offset | Size | Meaning |
| --- | ---: | --- |
| `00` | 4 | `EMOD` magic |
| `04` | 1 | format major (`1`) |
| `05` | 1 | format minor (`0`) |
| `06` | 2 | header size (`128`) |
| `08` | 3 | complete image size |
| `0B` | 3 | entry offset in image |
| `0E` | 1 | minimum Core ABI |
| `0F` | 1 | maximum Core ABI |
| `10` | 1 | provider class: `1` star command, `2` service |
| `11` | 1 | format flags (zero in v1) |
| `12` | 1 | namespace length |
| `13` | 1 | provider-name length |
| `14` | 16 | zero-padded lowercase namespace |
| `24` | 24 | zero-padded lowercase provider name |
| `3C` | 3 | provider semantic version |
| `3F` | 1 | reserved zero |
| `40` | 4 | capability bits (zero in v1) |
| `44` | 2 | minimum accepted request size |
| `46` | 2 | maximum accepted request size |
| `48` | 4 | payload CRC-32 |
| `4C` | 4 | header CRC-32 with this field zeroed |
| `50` | 48 | reserved zero |

Star providers require an empty namespace. Service providers require a
namespace. Identities use lowercase ASCII letters, digits, dot, underscore, and
hyphen; the first character must be a letter. The canonical registry key is
`(class, namespace, name)`.

The target provider entry uses the Zilog ADL C calling convention:

```c
UINT24 emos_provider_entry(t_emos_provider_request *request);
```

The packed provider request is 24 bytes: size `u16`, ABI major/minor `u8`, operation and
flags `u16`, input pointer/length `u24`, output pointer/capacity `u24`, mutable
output length `u24`, and one reserved zero byte. A provider may only retain
values copied from the request; every pointer and all module memory expire on
return.

The public Core gateway request is 66 bytes and carries the same buffer fields
plus fixed 16-byte namespace and 24-byte provider-name fields. Its only public
call target is the resident Core gateway; it never contains an external-module
entry point. EMOS v1 accepts only operation `2` (service) through this gateway
and requires a nonempty canonical service namespace.

The resident qualification-only `edu.text-probe` service is resolved by Core
without loading a module; its bounds and behavior are specified in
[the EMOS contract](../../docs/emos-v1-contract.md#resident-text-qualification-service).
The following header restrictions apply to transient-provider calls.

Advanced-header ADL applications declaring bit 0 are module-safe. Those
declaring bit 1 are module-compatible and require writable storage for Core's
private `/.emos-swap.bin` full-area save/restore transaction. Invalid inverse
flags, reserved bits, conflicting declarations, mismatched optional load
addresses, Z80 images, version-0/unheadered images, and moslets are denied.

`manifests/` freezes three bounded conformance identities. `hello` is the
generic star-command example, `core.echo` is a generic synchronous service,
and `edu.probe` is an Extender-shaped fake service. The EDU fake proves only
the namespaced service boundary and separate result domain; it is not an EDP
transport, physical-discovery, or firmware implementation.

Run `make -C projects/emos validate` to compile the runnable fixed-address
providers, wrap them with the canonical generator, verify their entry address,
and validate the resulting deterministic module directory. Generated media is
kept under the ignored `build/` directory.

The same target creates `build/media/cases/`. Its `valid` directory contains a
directory-backed SD root and `emos.commands` covering discovery, `HELLO`, both
service calls, fake Dual, and verified final Legacy recovery. `make
emos-runtime-check` executes that exact read-only transcript inside the target
MOS under headless Fab and proves the media was not changed. Separate roots cover a duplicate
claim and payload corruption; the eligibility case holds exact positive and
denied advanced headers. Copy a chosen case's `sdcard` contents into an
isolated emulator profile only for the required human launcher test. From the
repository root, `make stage-emos-media` performs that step with a complete
preflight: it accepts byte-identical staged files but refuses symlinks,
different files, stale providers, and unreviewed additions. It never replaces
local profile state or claims the fake is physical EDP firmware.
