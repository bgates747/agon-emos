# INTEG-014 dual-UART observation application

## Executive summary

This is a temporary E06 diagnostic, not a production application or public
EMOS API. It counts UART1 IRQ entries and observes raw UART0 replies without
changing the firmware parser. It binds to an exact ROM, verifies vector
ownership and restores original handlers before route changes or SD work.
Read [the frozen procedure](../../docs/tasks/INTEG-014/E06-procedure.md) before
use. Do not run concurrently with another bench workload or resident vector
owner. Hardware qualification is recorded in E06, not implied by building.

Use the repository `.venv/bin/python` to run `prepare.py --image <MOS.bin>
--elf <MOS.elf> --nm <toolchain/bin/ez80-none-elf-nm>`, then `make -C
projects/integ014-dual-uart AGONDEV_TOOLCHAIN=<toolchain>` and run `verify.py`.
Supply preserved E05 candidate artifacts, not whichever generated firmware a
builder currently happens to contain. Generated bindings, objects and binaries
are ignored. `prepare.py` verifies the pinned UART0 entry against its original
linked register-save and stock-read/parser call envelope.

`smoke` runs quiet return256 and forward4096. The default invocation runs three
repeats of quiet, 60Hz mode-query service and a256-record mainboard reply burst,
for return256, forward4096 and forward65535:27cases. Every forward case uses
256-byte chunks. `DUTnnn.CSV` contains timings, IRQ counts, exact-data errors and
restoration status; matching `.raw` files retain bounded UART0 observations.
A normal terminal row and visible pass/fail summary distinguish completion
from a stuck test. MOS-clock timings have16.667ms granularity at60Hz.

The external batch must configure both displays to mode20 before invocation,
return to Legacy/mode3 afterward and start the SD service. There is no mode
switch inside the app. Use the established guarded SD staging/readback and
CLI admission tools. Leave root startup unchanged.

UART1 observation adds only a preserved-HL counter then jumps to the original
handler. UART0 retains the full current register envelope and stock read/parser
calls, adding a bounded copy and IRQ count. Its ordinary ExCom effect-suppression
rule remains in force. Observation overhead is part of these diagnostic timings;
E05's uninstrumented wire metrics remain the production-path reference.

The private mainboard output call reaches the pinned existing EMOS routine;
it neither exports a new API nor changes the committed ordinary-VDU backend.
These address-bound internal calls are exclusively for controlled dual-UART
qualification. The application does not change UART registers or physical wiring.
Both temporary handlers are removed before normal ownership transitions. A
foreign vector is never overwritten: that exceptional condition leaves the
application stopped for inspection, without continuing the launch batch.
