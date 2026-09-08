# INTEG-007 — EMOS-owned General Poll diagnostic

Status: active; EMOS installed, paired hardware test pending. Started: 2026-09-08.
Coordinator: [PORT-013](../../../agon-extender/docs/tasks/PORT-013.md).

Author-approved v0.6.0 adds Legacy-only EMOS VDPPOLL at 1152000/8N1 with
RTS/CTS. Send `17 00 80 A5`; require `80 01 A5`, followed by a bounded quiet
interval. Keep state local, UART0/onboard VDP sysvars unchanged, and reject
occupied UART/RTS, malformed/extra replies, clock stalls and blocked timeouts.
The coordinator records official docs, tagged references and physical scope.

1. [x] Implement the command using existing private UART polling helpers and
   test real command success/failure/cleanup paths.
2. [x] Run full configured/linked/runtime gates, host tests and same-build
   smoke/no-peer review; obtain Author graphical acceptance before commit.
3. [ ] Consume the coordinator's physical evidence before closing this task.

Implementation gotchas: the private driver distinguishes UART_POLL_BLOCKED
(CTS stop) from UART_POLL_EMPTY (FIFO wait); both must remain bounded waits.
The command harness includes stuck and subsequently released CTS. The parallel
source verifier formerly rejected `general_poll` even in Core diagnostics;
it now admits only the explicit Legacy VDPPOLL call while rejecting additional
Core calls or any occurrence in parallel/serial sources. Mutation tests retain
that ownership boundary. The linked baud and Port C writer guards are unchanged.

## Draft review checkpoint — 2026-09-08

EMOS `agon-emos-v0.6.0-b2026-09-08-17-43-51Z` passes the complete
configured qualification gate, linked UART-divisor/ABI/VDU/parallel checks,
runtime regression and all 71 host tests. The General Poll harness covers
14 cases including CTS held then released, stuck CTS, wrong/short/extra reply,
clock stall and cleanup. Ordinary and bad-SD smoke pass for stock and EMOS;
the no-peer review fails within its deadline and returns to the prompt.

P4 `uart-general-poll-probe-r01-b2026-09-08-17-41-34Z` builds successfully;
its linked symbols include retained processNext, sendGeneralPoll and send_packet.
The actual admission/return model's sanitized tests pass, as do all 21 UART
capture-checker tests (including three new General Poll/completion tests).
Registry r31 artifact validation passes; the unrelated held-r02 connectivity
hash mismatch remains as previously documented.

Graphical review is ready for Author validation. Neither candidate source is
committed; identities remain draft. No hardware flash/reset, analyzer acquisition
or SD modification occurred. Private bundles and capture preparation live under
the coordinator's ignored agents/general-poll directory.

## Author graphical observation — 2026-09-08

The Author supplied a screenshot of draft
`agon-emos-v0.6.0-b2026-09-08-17-43-51Z` showing SD/CLOCK PASS,
`VDP POLL: 1152000 baud`, `VDP POLL FAIL: transmit timeout`, and the final
MOS prompt. This matches the expected no-peer outcome. The accompanying
`Volume timeout` is MOS's generic FR_TIMEOUT text, not an SD failure.
Explicit source-freeze approval is pending; no physical result is inferred.

## Source freeze authorized — 2026-09-08

After the recorded graphical review, the Author approved proceeding with
source freeze and clean candidate preparation. Promote v0.6.0 and the General
Poll fixture lifecycle metadata to candidate with reviewed implementation
unchanged. Commit before building and retain passing v0.5.0/flow-r02 rollback.
Physical flashing remains subject to the recorded bench authorization boundary.

## Clean candidate prepared — 2026-09-08

Source freeze `e5d9921` with unchanged builder `cf24304` produced candidate
`agon-emos-v0.6.0-b2026-09-08-17-50-12Z`. Full configured/linked/runtime
checks, 71 host tests, stock/EMOS ordinary and bad-SD smoke, and the bounded
no-peer General Poll check pass. The firmware is 120901 bytes, CRC32
`642B19C0`, SHA256
`89a71acdd5e4e076e74c0865ecf2c2cd1d9b2f69f00135992dba204742880123`.

The coordinator's PORT-013-2026-09-08-17-55-37Z record retains the exact
build manifest and verified installer media. SD is safely unmounted; working
v0.5.0 is retained as EMPREV.BIN and off-card. Author-observed installation,
same-build smoke/VDPPOLL media handover and paired hardware testing remain
pending. No physical General Poll result is inferred from emulator checks.

The Author reported successful v0.6.0 installation. The consumed SD payload
matches the candidate hash and EMNEW.BIN is absent. Coordinator record
PORT-013-2026-09-08-18-06-37Z retains verified same-build smoke/VDPPOLL media
preparation and safe unmount. The paired General Poll result remains pending.
