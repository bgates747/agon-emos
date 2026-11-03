# MI-005: matched performance fixture

The question is how much application work the small sender frees up, and
whether OTIRX improves on its otherwise identical scalar refill. These are
experimental Eirene images, not approved stock MOS. Firmware remains exactly
the three images pinned in functional-r02.md; this tranche adds only an
original AgonDev C application and its timer/syscall bridges.

PF-001 [x] Freeze the comparison: pristine Eirene ordinary RST18, lightly fixed
Tom-derived scalar, and that same sender with OTIRX. One app binary takes mode
0, 1 or 2 respectively. Before any experimental call it checks the installed
ROM prefix length/CRC32 against the selected, SHA256-pinned image. Unused flash
tail is excluded so emulator zero fill and physical erased flash agree.

PF-002 [x] Build identical traffic: 8192 deterministic payload bytes, wrapped in
32 public buffered-VDP append commands of256payload+8header bytes (8448wire
bytes). Bulk submits the whole stream once. Paced submits32chunks at rational
256/44100-second deadlines; rate means **44100 payload B/s**, 45478.125wire B/s.
This is chunk pacing, not44100single-byte IRQs per second or audio latency qualification.
Only one source is outstanding. Async callback merely counts and posts done;
the flag is armed before submission. It stays open between paced chunks and
finishes after the last one. Ordinary output retains its synchronous behavior.

PF-003 [x] Measure PRT1 ticks at1152000Hz and a separate PRT2 counter at72000Hz;
claim only initially disabled timers, leaving PRT0 alone. Preserve vectors and
source selection on return; subsequent timer owners configure their reloads.
Check the counters'16:1 relationship, not the accuracy of a calendar or monitor.
A no-traffic calibration counts fixed xorshift work quanta in the same213996tick
observation window. Paced trials count that work while waiting for deadlines
and source completion. Record the state checksum, missed completion deadlines
and worst observed submission lateness. Work counts are a foreground-capacity
proxy, **not direct CPU-cycle attribution**; ISR and polling costs are included.

PF-004 [x] Keep boundaries explicit: `call` sums time inside submission wrappers;
`source_observed` is foreground observation after the final callback (ordinary
output's return), not an ISR timestamp. `ticks` ends at local UART TEMT/finish,
including the paced observation window. `fence` is a separate later GP-query
roundtrip; it is excluded from local-drain throughput. Bulk does no useful-work
loop. PRT reads/wait control are identical instrumentation across variants.
Three repeats alternate bulk/paced order. Bulk is a short8KiB sample, not a
claim of unlimited sustained throughput under every VDP workload.

PF-005 [x] Receiver verification, screen breadcrumbs, file reads and logging are
outside timed sections. Negative/positive controls prove the public VDP byte
comparator. Verify every one of8192received bytes after each of6trials. A GP
fence before each interval prevents prior printed text from contaminating it.
Use create-new Rnnnn.log files, next.txt and an append-only runs.log; never infer
run order from Agon's clock. Do not free or exit with an uncertain live source.

PF-006 [x] Run the6finite actual-firmware Flexe cells (three images ×JIT/interpreter)
through the shared Oryx lease. Check report counts, deterministic work states,
clock agreement, replayed payloads/comparisons and runtime/UART health. Preserve
all attempts; fail-stop the cohort, release before offline analysis. Emulator
numbers validate the fixture only; physical Console8 decides speed claims.

PF-007 [x] Prepare the matched manual Console8 appliance, with the same test.bin,
checks.bin, all three clearly named MOS images, matched VDP, stock restoration
files, short readme.txt and provenance. No SD write, physical flash or claimed
hardware result is part of this preparation. TM-005 remains open for the Author's
three firmware selections/runs and our returned-report analysis.

Final evidence: [performance-r02.md](performance-r02.md). MI-005 is complete;
TM-005 remains open for manual Console8 measurements. No SD card was written.

Physical follow-up is complete: [hardware-r01.md](hardware-r01.md). All three
requested selections passed; TM-005 is closed. Application validation is next.
