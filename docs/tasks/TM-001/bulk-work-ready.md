# Five fixtures ready for Console8 — 2026-11-03

Each of the five MOS images now has its own app that runs every scenario with
no switches. The new bulk-plus-calculation scenario measures whether useful
foreground work can continue during flat-out transmission and what that costs
in throughput. The firmware images themselves are unchanged.

| Flash from /test/mos-uart0 | Then invoke after reboot |
| --- | --- |
| `flash mos mos-stock.bin` | `stock` |
| `flash mos mos-eirene.bin` | `eirene` |
| `flash mos mos-scalar.bin` | `scalar` |
| `flash mos mos-otirx.bin` | `otirx` |
| `flash mos mos-ours.bin` | `ours` |

Use the matched vdp-test.bin already staged in the same directory; if the VDP
has changed since the last UART run, flash it before this cohort. Each app
checks its MOS ROM identity, performs three repeats each of bulk-only,
paced-plus-calculation and bulk-plus-calculation, checks every received payload
byte, writes a sequential report and requires PERF2 PASS. Allow about a minute
per app; longer than two minutes is a failure, with its last breadcrumb needed.

All ten Flexe cells (five firmware/app pairs, both engines) passed using the
plain MOS command names. Independent validation checked all90trials and737280
receiver payload bytes, work-state checksums, callback counts and UART/backend
fault counters. Main and timer C object files are byte-identical across all five
builds. The report consumer also passed against a private five-case model copy.
These are functional/model checks, not physical performance measurements.

The Console8 card AGONC8, UUID8658-1C23, was backed up and the archive verified
on Pi and Oryx before replacement. All70 deployed files were checked after
writing and flushing. The complete previous appliance is retained in previous/;
R0042..R0046 also remain in the active directory. Numbering continues at R0047.
All five NEW apps must run on hardware; old PERF1 counts cannot fill the new
scenario. PERF2 uses one xorshift step per work unit instead of sixteen.

Source commit: `79e830af6f7a250a0bbdce83fb856f166a3cd68c`.
[Machine-readable image, app and evidence pins](bulk-work-ready.json).
[Measurement contract and reproduction](../../../experiments/uart0-performance2/README.md).
[Task](bulk-work.md). Frozen workspace evidence is in
build/tm001/perf2-001, perf2-flexe001 and perf2-deploy001 on Pi and Oryx;
the latter includes before.tar.gz and deployment.json. Emulator runtime/cards
remain on Oryx; raw qualification reports and streams are also preserved on Pi.

Bulk-work credited counts discard the final potentially straddling work unit;
raw count/state remain available. Completion observation includes the final
work quantum and drain-check overhead. The measurement describes useful
foreground work under each API, not exclusive CPU-cycle accounting. Submission
call time does not include all interrupt-service CPU cost. Local source release,
UART drain and receiver execution are reported separately. Functional PASS
is not a claim of a speedup or a real-time deadline.

Checkout228 was released with fresh whole-host idle evidence. The next owner,
agon-6502, has checked out generation229. Its handoff was delivered in the
durable mailbox; this session exposed no immediate chat-sending tool.

Next: Author runs all five fixtures; archive the returned card and compare
throughput, useful-work rate, no-traffic calibration fraction and deadline
misses. No new hardware result is claimed yet.
