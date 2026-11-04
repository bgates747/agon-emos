# TR-001 Console8 comparison

Everything is in `/test/mos-uart0` on AGONC8. Use stock Bistromathics VDP2.16.0
for every configuration. If needed, from that directory use
`flash vdp vdp-stock.bin`, allowing the updater to finish and reboot normally.
The installed standard flash MOSlet is sufficient.

After each MOS flash, let it reboot, then return to `/test/mos-uart0`.
Type each application name directly, with no arguments.

| Configuration | Flash command | Applications to run |
| --- | --- | --- |
| Official stock MOS3.0.2 |`flash mos mos-stock.bin`|`stock`|
| Published Eirene UART0 RC1 |`flash mos mos-rc1.bin`|`checkrc`, then `rc1`|
| Smaller unpublished trial |`flash mos mos-trim.bin`|`test` twice, then `example`, then `trim`|

Before the trial apps, confirm `Eirene UART0 TRIM MOS Version 3.1.0 Trial 1`,
power off/on once, and confirm the normal prompt returns with the same banner.
Its ROM CRC32 is D5CC99AE. RC1's banner is Eirene UART0 MOS3.1.0 RC1 and ROM
CRC32 is3F792F8D. Apps check their exact firmware before experimental calls.

Require RC1 PASS from checkrc; TRIM1 PASS from both test runs;
TRIM1 EXAMPLE PASS from example; PERF2 PASS from stock/rc1/trim, each returning
to the normal prompt. Allow two minutes per application. A stall is failure;
retain its reports and note the last progress line. There is no automatic
firmware flasher or interactive menu in this focused comparison.

Each performance app runs bulk-only, paced+calculation and bulk+calculation,
three rotated8192-byte repeats each, with full receiver checks. The test fixture
checks callbacks, boundaries, BUSY/NULL, source reuse, concurrent RX, ordinary
output and teardown/re-entry. It does not test removed context rejection.
Return the SD to Oryx after all configurations. Sequential logs continue at
R0054; the example has no report. No timestamp is used.

The trial may stay installed while Console8 is dedicated to UART testing.
Restore when needed with `flash mos mos-stock.bin` and stock VDP. This trial is
unpublished; published RC1/main/tag/assets remain unchanged. Hardware stability
and performance of the new trial remain pending this run.

Run completed: [R0054–R0059 results](hardware-results.md). All expected logged
cases passed; report60 is next. The procedure above is retained for reference.
