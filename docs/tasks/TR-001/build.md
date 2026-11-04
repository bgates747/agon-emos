# TR-001 assembled candidate

This trial keeps the background sender while cutting its MOS instructions by
40%. It trusts a documented caller contract and moves extra C callback saves
into the application. It is unpublished; hardware performance remains pending.
Published RC1, its assets/tag and main are unchanged.

| Component | Published RC1 | TRIM Trial1 | Difference |
| --- | ---: | ---: | ---: |
| Sender instructions |476|286|-190 (39.9%)|
| Persistent RAM |11|9|-2|
| Complete branded MOS image |101140|100958|-182|
| Matched benchmark ABI bridge instructions |225|266|+41 in the app|

The fixed-target C callback gate itself is29 bytes; the benchmark's argument
bridge adds another12 net bytes. The complete benchmark images differ by50
bytes because their labels/configuration strings also differ. A normal C app
can pass its gate pointer directly without that benchmark-only selection code.
The savings are not all eliminated code: do not hide the moved application cost.
Tom's supplied sender is158 instruction bytes plus9 inline variable bytes
(12 bytes of persistent state including its overlaid vector). Our trial still
has128 more instruction bytes than his example; it retains MOS integration,
nonblocking CTS handling, RX chaining, boundary/accounting and finish behavior.

Controlled steps with identical trial branding:476 ->402 after removing broad
context/foreign-owner checks;315 after simplifying syscall wrappers/session
state;286 after moving callback saves and replacing indirect-call scaffolding
with a tail call. See build.json for all images and object hashes. The8-byte
branding increase explains why whole-image saving differs from instruction saving.
The trial is298 image bytes larger than pristine Eirene3.1.0_beta_1 (100660bytes).

Source: df2066832ecb6a6a33a817acb8b9f41d609e44cc on codex/uart0-async.
MOS CRC32 D5CC99AE; SHA256
894ea631d7f2cce7997f1bcfcede07fe988bcf2f2adf1127dcf16d4e654b0316.
Oryx build: build/tm001/tr001-002, mirrored under the same project-relative
path on Pi. Both clean builds are byte-identical. The first and corrected
source snapshots produce the same MOS; the correction only fixes an app gate.
All other MOS source files except version.h/uart0_async.asm are unchanged against
RC1; compiled objects except main.o/uart0_async.o match RC1 byte for byte.
Startup, reset, RST18 and receive-parser source are unchanged. Both full builds
retain the exact same31 upstream warnings as RC1; C fixtures build with no warnings.

The three new benchmark apps share identical main.o/platform.o. Each invokes
all three PERF2 scenarios with three rotated8192-byte trials, receiver checks,
clock checks, calibration and checksums. Stock and RC1 remain exact frozen
firmware, and trial pays its wrapper cost. Older measurements are retained;
these new fixture identities must accompany any new hardware comparison.

Caller changes: foreground ADL1/MBASE0/EI, normal MOS UART configuration and sole
UART ownership are preconditions. Broad validation/context rejection is removed.
Raw callbacks preserve IX/IY/alternate banks if touched; fixed-target C gates do
that for C callers. Normal MOS syscall/ISR register preservation remains. No
queue, implicit exit cleanup or release publication was added.
