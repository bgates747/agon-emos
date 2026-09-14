# E04 — Controlled original-EMOS baseline

Author authorised E04 on 2026-09-14. This bounded procedure extends the existing
INTEG-014 checklist; E05 remains a separate supervised gate. Preserve existing
dirty source. No EMOS optimisation or experimental push in this step.

## Fixed inputs and interpretation

Reuse Extender PORT-008's frozen pure-data procedure and wire procedure, with
its exact previously built application and paired probes. Original EMOS stays
at the E02-reproduced bench image, SHA256
`1da8330462eed54317e5889cf3f09cb1abaca8b478f7d92b88a3a1d74c9fc70c`.
No telemetry producer runs. This baseline does not independently measure the
ordinary composition or stock MOS firmware: route0 uses EMOS's retained stock
UART0 path, route1 its current ExCom path. Report that boundary explicitly.

| Artifact | Exact identity / SHA256 |
|---|---|
| Pure-data app | `uart-data-probe-r01-b2026-09-13-21-30-31Z`; `7fd452a40912b103d21a1c0dcedaa56351b692ef25c78ebdae427c3a4f017fdc` |
| Mainboard stock VDP 2.16.0 plus data probe | `uart-data-probe-r01-b2026-09-13-20-18-15Z`; `1c5b15b35d277e8f1af8f68023bea95807f11d120e557eff7d2cadb0655dd71d` |
| Aligned P4 app | `uart-excom-console-r17-b2026-09-13-21-13-50Z`; `6dcb412c70a5af3468d438126376667e3d54cded28c92f6b893c6a618660bffe` |
| Aligned P4 factory image | `803905e615e46a78aca831ab5a10365c60ab707ce1abe243210cc1e3171b0520` |

The fixed mainboard probe is needed for destination timing/exact readback;
“stock” in result tables means stock VDP transport plus this same observer,
not an uninstrumented release. No rendering, browser output, serial observer,
SD access or independent input producer inside a timed interval. Select both
display modes externally using the existing batch (mode20, 60Hz); restore
mode3 afterward. Physical wiring and pinwalk-confirmed analyzer map unchanged.
Machine identities and exact bench journals stay in ignored local records.

## Sequence

1. Preserve current startup/prior backup and SD/keyboard state; check artifact
   hashes and recoverable original firmware. Freeze this procedure before
   deployment. Preserve/readback actual mainboard flash and verify expected
   prior P4 application/partition before replacing it.
2. Deploy the fixed probe pair through existing guarded tools in new evidence
   directories. Independently verify flash. Perform one explicit mainboard
   reset after P4 startup, admit fresh neutral keyboard, verify SD read/write
   and unchanged startup. No blind reset loop after failure.
3. Reuse exact app: full separate336, mixed36, full separate336, mixed36.
   Each matrix already contains three repetitions. This supplies six samples
   per matching cell across two invocations. Each result needs exact row order,
   byte counts/patterns, zero error/status and terminal recovery0. Collect SD
   files between invocations only; retain each failure before diagnosis.
4. Three short wire invocations, four rows each, using existing 24MHz/30second
   capture and independent decoder. Same all-UART-wire map and payload oracle;
   no new marker or altered protocol. Keep host orchestration duration,
   MOS-clock ticks, endpoint micros and wire windows distinct. Verify no new
   P4 frame snapshots throughout; poll service sparsely outside wire capture.
5. Analyse repeatability and freeze numerical improvement thresholds **before**
   E05. For high-resolution endpoint/wire measurements, require at least
   max(5%, twice the observed range divided by median) reduction in the same
   metric, with zero correctness regression. Report measured values and noise;
   three wire samples are an initial practical bound, not a confidence interval.
   MOS-clock claims additionally must exceed two 60Hz quanta (33.333ms), or use
   the high-resolution paired metric instead. Do not use enqueue-only reverse
   timings as end-to-end return latency. Zero/no-op controls are not subtracted
   automatically. Mixed forward timing includes preceding reply work.
6. Restore exact original P4 and mainboard firmware, covering every touched
   erase sector. Re-admit normal boot, verify startup/prior backup, SD roundtrip
   and released keyboard. Leave ordinary MOS prompt. Record tabular findings,
   commit E04 completion, play the standard British hardware alert and stop.

Host staging/capture helpers may be copied into new ignored run directories
with only explicit paths/run names adapted; do not edit retained earlier
receipts or overwrite their output. No source change means no new build ID;
new executions receive fresh run directories and timestamps. Any new failure
or discrepancy is preserved and investigated before advancing. If a fixed
input assumption fails, record the deviation before further physical work.

## Preflight observation

Fresh independently verified mainboard flash differs from the previous UART
pass's pre-test backup only in five sectors beginning at `0x3f0000`. The actual
partition table identifies these as **coredump**, not application settings.
Boot, partition, application and all bytes below `0x3f0000` match the previous
stock-restored image. The dump's originating incident is not identified by this
comparison; do not attribute it to E04, whose probe has not been deployed yet.
Preserve the new full backup and the sector-difference record. Deployment touches
only the application region; restoration uses today's preserved bytes and does
not erase this diagnostic evidence. No firmware baseline assumption changes.
