# TR-001 qualification and deployment

The smaller candidate and matching stock/RC1/trial applications are ready for
the physical comparison. No new hardware performance result is claimed.

Source-pinned build: df2066832ecb6a6a33a817acb8b9f41d609e44cc.
Complete build and app identities: build.json and build/tm001/tr001-002.
Private frozen source/evidence/appliance package: build/tm001/tr001-package001,
mirrored and SHA256SUMS-verified on Pi and Oryx. Full raw model evidence remains
in that package; readable reports are also retained in model-reports/.

Eight finite Oryx Flexe cells passed: trial regression twice plus its C example
in both JIT/interpreter, and stock/RC1/trial PERF2 in both engines. All six
benchmark cells passed all nine trials, receiver verification, callback counts,
xorshift checksums, and independent-clock consistency. Verified payload bytes:
475136 across the successful campaign. UART drop/overrun counters and backend
failed/unhandled/unregistered/unmapped counters were zero. Keyboard hooks were
installed, but keyboard events were not injected. Model timing is not a physical
speed result. The runtime uses the previously pinned receiver-fixed VDP for
these functional checks; the physical comparison deliberately uses stock VDP.

The first campaign is retained at build/tm001/tr001-functional001. Both focused
regressions passed, then its standalone example panicked: the new gate include
lacked ADL1 and encoded CALL with a16-bit target. Adding explicit ADL1 corrected
the application wrapper; the MOS binary stayed identical. The preparation now
asserts the29-byte gate, and the runner stops on guest panic. No faulty example
was deployed. First lease generation237 released after that failure; corrected
generation238 released after all cells passed. No next waiter was present.

Deployment: /run/media/smith/AGONC8/test/mos-uart0 on Oryx, verified label AGONC8,
vfat UUID8658-1C23. The old folder was copied, checksummed and tar-verified at
build/tm001/tr001-deploy001 before replacement; both archive and receipt are
mirrored on Pi. R0052/R0053 and previous firmware/fixture history remain there.
The existing next counter54 and full1497-byte running log were preserved.
The active directory has28 required files, including matching binaries/metadata,
licenses and concise readme; old standalone test VDP/map/history files were
removed from the card after archive verification. Only this UART directory was
modified. Root MOS was not touched; no physical firmware was flashed, and no
remount/eject was attempted. See deployment.json for all file hashes.

Follow [the hardware protocol](hardware.md). Expected new successful reports
are R0054..R0059 if each command is run once as prescribed (test runs twice).
The example prints a result but writes no report. Continue numbering for any
retry; retain failed/incomplete attempts. Final analysis must match each new
build/ROM and compare the same stock-VDP trial conditions, keeping the older
receiver-fixed hardware series distinct.
