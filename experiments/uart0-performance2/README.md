# Five no-argument UART measurement fixtures

Build five original AgonDev C apps against the same compiled measurement core.
Type stock, eirene, scalar, otirx or ours at the MOS prompt, in the appliance
directory, after manually flashing its matching MOS. No LOAD/RUN or switches.
The app checks the installed ROM before calling any experimental interface.
MOS and VDP images are reused unchanged from their pinned qualified artifacts.

Every app performs three repeats of bulk-only, paced-plus-work and
bulk-plus-work, rotating order between repeats. Payload/framing, source
boundaries, callbacks, clocks and receiver checks are matched. Bulk-plus-work
continues calculating through nonblocking wire-drain checks, not just source
release. Logging, file reads, receiver comparisons and the remote GP fence
remain outside the local-drain throughput interval.

PERF2 work units are one xorshift step, versus sixteen in PERF1, to reduce
completion-observation granularity. Raw work/state are preserved and independently
checked; the last bulk-work unit may straddle physical completion, so credited
work conservatively discards it. The observed duration can include the remaining
fraction of that quantum and the drain-check cost. This measures useful
foreground capacity under the callable API, not exclusive CPU-cycle attribution
or hypothetical work in a separate application ISR. A blocking call can prevent
foreground work even when it spends time waiting on UART hardware.

The external configuration object prevents compile-time specialization. The
complete main.o and platform.o hashes must match across all five builds.
Callbacks and PRT snapshots retain the same register/IFF rules. The previous
full candidate uses its UTX1 borrowed/direct path, batch16, copied RST18 OFF;
its adapter/session costs occur in their actual paths and are included.

On Oryx:

```
python3 experiments/uart0-performance2/prepare.py build/tm001/NEW
AGONDEV_TOOLCHAIN=/path/to/agondev sh experiments/uart0-performance2/build.sh build/tm001/NEW
python3 experiments/uart0-performance2/stage.py NEW COHORT
```

Stage creates stopped guarded profiles with autoexec containing only CD, the
fixture command name, DIR and a return marker. Follow the canonical shared
Oryx FIFO; run only with the asserted checkout and clean up owned processes.

```
UART0_ORYX_LEASE=TOKEN python3 experiments/uart0-minimal/run_flexe.py COHORT
python3 experiments/uart0-performance2/analyze.py build/tm001/COHORT
```

Ten cells (five images ×two engines), nine trials each:737280receiver bytes.
Independent analysis validates each emitted packet/comparator command and parsed
work checksum, clocks, callbacks and identity. Backend faults and UART
loss/overrun fail qualification. Model timing is not hardware performance.

After a verified backup of the prior card to DELIVERY/before.tar.gz and
before.json, package.py BUILD DELIVERY builds an appliance with the old complete
package in previous/, the five new apps, all matching firmware, concise readme
and continuing report numbers. It does not itself write the card. Prior reports
are also retained in the main directory; the new parser ignores their PERF1
counts and never merges them into PERF2 measurements.

Archive/copy returned reports to the project workspace before analysis:

```
python3 experiments/uart0-performance2/summarize.py ARCHIVED_DIRECTORY --kind hardware
```

The result reports useful-work rate/fraction and throughput for each scenario,
plus bulk-throughput change caused by doing work during bulk transmission.
Failed/incomplete attempts remain visible. No division-by-zero percentage is
invented when stock completes zero useful work. Functional PASS is not a claim
that a particular performance target or deadline was met.
