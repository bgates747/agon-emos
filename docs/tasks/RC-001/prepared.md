# RC1 prepared and deployed — 2026-11-03

Historical preparation record. Current status: [published RC1](publication.md).
The final run used stock VDP; see [returned hardware evidence](hardware.md).

The selected minimal OTIRX implementation is frozen, branded and packaged as
Eirene MOS3.1.0 UART0 RC1. It remains on codex/uart0-async; main is unchanged.
The [GitHub prerelease draft](https://github.com/bgates747/eirene-mos/releases/tag/untagged-b2b61fb060995849c351)
is prepared with its ZIP and checksum asset. It is not published. Exact-image
Console8 qualification and subsequent publication are the remaining tasks.

Source commit:47dd56129c0043931bb32f7d329c0392a0b8a809.
Proposed tag:3.1.0_uart0_rc_1, targeting that commit rather than main.
MOS:101140bytes, CRC323F792F8D, SHA256
49873fb817f47cc76f50b8cec1c3d9cde7b252ac2ec844e08e745bdaf1c5eeab.
The literal boot line includes `Agon Eirene UART0 MOS Version 3.1.0 RC 1`.

Two clean firmware builds produced identical bytes. Compared with the
hardware-qualified pre-branding OTIRX snapshot, the only firmware source change
is src/version.h and the only differing compiled object is main.o, which emits
the banner. The sender object and all other MOS objects are byte-identical.
Startup/reset, main.c, ordinary RST18 and receive-parser sources are unchanged.
The exact31inherited compiler diagnostics match pristine Eirene's full build;
the new C example and regression fixture compile warning-free.

The Author waived branded-image emulator testing in favor of the final C8 run.
Reservation c9581db56613 was cancelled without a checkout or emulator process;
there was no next waiting request to notify. Prepared stopped profiles are not
execution evidence. The original implementation's earlier model/physical
qualification remains documented separately; no qualification is invented for
these newly branded image bytes.

## Package and card

The ZIP includes complete frozen MOS source and binary/map, the generated
no-argument test fixture and small original C example (source and binaries),
receiver comparator, known stock MOS restoration binary, manifest, compiler
identities, licenses/credits, caller documentation and earlier hardware evidence.
Its README explicitly records that exact-image hardware checking is pending.
The diagnostic VDP is not newly distributed in the public MOS ZIP; its matched
binary and stock restoration are present in the private Console8 appliance.
The UART sender introduces no new VDP protocol.

Prepared ZIP on Pi and Oryx:
build/tm001/rc1-package001/3.1.0_uart0_rc_1.zip, SHA256
773ed19618eb4eb9029b3aa256adbbc77ad6d0f5172c785394fdfdcacdec1449.
GitHub's uploaded-asset digest matches. Before publication, add returned
exact-image evidence/update the pending status and finalize the ZIP/checksum;
do not alter the MOS source/binary silently. The frozen public source commit
also contains the original optional model recipe; the Author waiver and current
hardware-only packaging mode are recorded in the maintained branch task.

AGONC8 UUID8658-1C23 now has21verified files under /test/mos-uart0. The complete
previous appliance was checked against the returned-run snapshot, archived,
verified on both machines, then replaced. No earlier apps/versions accumulate
in the active directory. Reports continue at R0052; runs.log retains the journal
and a new RC campaign note. No root MOS.bin, autoexec or other SD directory was
changed. The card was flushed and left mounted for normal Author ejection.
Before archive: build/tm001/rc1-package001/before.tar.gz, SHA256
f11003239b7db39018ef6684e9535bf7a1091db90cd570c86c9274c6099fdebd.
All earlier reports and sources remain in the project archives.

## Manual check

From /test/mos-uart0:

```
flash mos mos-rc1.bin
```

Reboot as prompted, then power off/on once. Confirm the RC banner above.
Retain the matched VDP from the last UART run; if replaced, use the staged
`flash vdp vdp-test.bin` and reboot normally. Return to the same directory:

```
test
test
example
```

Wait for RC1 PASS and the ordinary prompt after each test, and RC1 EXAMPLE PASS
from the example. Each test must finish within two minutes; report its last
breadcrumb if it stalls. No switches or LOAD/RUN. The tests check ordinary
output before/after opt-in, callbacks and NULL callbacks, boundaries, BUSY,
source reuse, concurrent RX and8192payload bytes per invocation. Source/code
are retained on uncertain teardown. Tell the agent that the cold-boot banner
and example passed, then return the card for R0052/R0053 analysis. No stock
restoration is required while the board remains dedicated to these UART tests.

[Full pins, object/source identities and deployment receipt](prepared.json).
[Promotion tasks](README.md). All independent preparation is complete;
RC-003's physical result and RC-005 publication remain open.
