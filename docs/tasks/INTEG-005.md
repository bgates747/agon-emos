# INTEG-005 — Bounded UART flow-control diagnostic

Status: implemented; automated and Author-supplied graphical checks passed;
source freeze and candidate deployment approved.
Started: 2026-09-08.
Coordinator: [PORT-011](../../../agon-extender/docs/tasks/PORT-011.md).

1. [x] Extend internal polling helpers to honor CTS when configured. Add
   explicit PC2 RTS ownership for an EMOS-opened polling UART; reject an
   occupied pin/UART and release the added output on cleanup. Preserve public
   MOS APIs, UART0 and lifecycle exclusion with parallel ownership.
2. [x] Add Legacy-only EMOS UARTFLOW at 115200/8N1. Exercise peer pause/resume,
   hold/release the ACK, detect permanently blocked transmission, and return
   with finite deadlines even when MOS's clock stops.
3. [x] Test real driver and command logic for exact reply, corrupt/extra/early
   bytes, peer silence, errors, blocked waits, occupied pins/UART and cleanup.
4. [x] Run the complete configured build gate and same-build ordinary/bad-SD
   and UARTFLOW no-peer reviews. Obtain Author visual review before committing.
5. [ ] Build/install reviewed clean candidates and consume physical evidence
   from the coordinator before completing this task.

The Author approved v0.4.0 for draft review. Existing v0.3.0 remains installed.
Official MOS API docs describe the settings struct but not a complete four-wire
UART1 implementation. Stock uart.c selects PC3 as GPIO CTS and serial.asm waits
without a deadline. EMOS's private helper supplies the bounded CTS poll and
explicit receive-ready output; standard blocking APIs retain their semantics.
Official checkouts remain read-only. See the coordinator's bounded references
and fixture contract; no new mode or production transport protocol is added.

Build-check gotcha: the established UART mutation verifier delimits open_UART1
using close_UART1. Keep new helpers outside that interval; the original
17-mutation guard and linked checks remain unchanged.

The whole-image Port C writer inventory now admits the three explicit RTS
helpers/cleanup owners, each restricted to its exact PC_DR/PC_DDR write
sequence. Existing startup, UART-open and parallel-owner checks remain.
Real-driver host tests cover the PC2 masks, ownership and cleanup semantics.

Draft agon-emos-v0.4.0-b2026-09-08-05-12-13Z passes the configured qualification
gate, linked ABI/VDU/UART-baud/parallel checks and runtime regression. All 68
host tests pass when the repository test target selects that prepared EMOS
worktree. Ordinary and bad-SD cases pass for stock MOS and EMOS. The combined
smoke/UARTFLOW no-peer review returns `peer did not hold CTS` and the prompt;
Fab supplies unheld CTS here. The generic `Volume timeout` is MOS's existing
FR_TIMEOUT wording, not an SD failure. New command tests cover 15 scenarios.

The Author supplied the graphical review and subsequently approved committing. No physical
deployment occurred; installed v0.3.0 remains the working image. Review bundles
and launcher are recorded in the coordinator's ignored uart-flow handoff.

The Author screenshot shows the same v0.4.0 draft, SD/CLOCK PASS, the expected
`CTS did not release` failure and final prompt. This graphical outcome differs
from the CLI's unheld-CTS case; both are accepted by the prepared no-peer
checker. No measured wall-clock duration or physical RTS/CTS result is claimed.

The Author approved source freeze and candidate deployment on 2026-09-08.
Promote v0.4.0 lifecycle metadata to candidate with the reviewed implementation
unchanged, commit, and build through the existing complete gate. The previous
v0.3.0 and v0.2.0 images remain available for rollback.
