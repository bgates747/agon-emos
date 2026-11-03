# Console8 performance appliance deployment

The Author requested SD deployment, replacement of the old UART fixture and
inclusion of firmware. On 2026-11-03 the qualified PERF1 appliance was deployed
to `/test/mos-uart0` on Oryx's `AGONC8` card, UUID 8658-1C23. This is the same
build 1ad95f651c6e017d and test.bin qualified in performance-r02.md. No firmware
was flashed and no hardware result is claimed.

DP-001 [x] Archive all old UART files, logs, autoexec.txt and root MOS.bin before
replacement. Both Oryx and Pi hold verified before.tar.gz copies under
`build/tm001/deploy-console8-r01`, SHA256
9c5ccaa0f4a26ac0d9c90f7e53ac20965c08766ab98056004e17ae9742db848b.
The old balls and callback directories are removed from the card; history
remains in the project workspace.

DP-002 [x] Deploy test.bin, checks.bin, pristine/scalar/OTIRX MOS, matched
receiver-fixed VDP, stock MOS/VDP restoration files, concise readme.txt,
provenance, licenses and checksums directly in the one active UART directory.
Root MOS.bin is the unchanged stock restoration image; the instructions use
explicit filenames in the appliance. Unrelated card applications remain intact.

DP-003 [x] Continue numbering at R0042 using next.txt, with a concise campaign
runs.log. Preserve the old startup script in the archive and set autoexec.txt
to SET KEYBOARD 1 plus CD /test/mos-uart0. Flush the volume, verify all 22 deployed
files by SHA256/readback and verify no old UART subdirectories remain. Do not
unmount/remount the card; the Author handles ejection.

DP-004 [x] Author: flash vdp-test.bin once, then run each MOS selection in order:
mos-eirene.bin with `run . 0`, mos-scalar.bin with `run . 1`, and mos-otirx.bin
with `run . 2`. After each MOS flash/reboot, load test.bin afresh in
/test/mos-uart0.
Allow two minutes per run; require PERF1 PASS and return to the prompt. Leave
all Rnnnn.log/next.txt/runs.log files together for returned-card analysis.
TM-005 closed after inspection of R0042..R0044; see hardware-r01.md.

The JSON companion records every deployed file hash, source commit, archived
state and card identity. The fixed deployment has 22 files, including next.txt,
runs.log and deployment.json. No obsolete test binaries or bundled flash copy
remain in the UART directory; use the installed standard flash MOSlet.
