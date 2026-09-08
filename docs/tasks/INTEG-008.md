# INTEG-008 — EMOS-owned visible-text diagnostic

Status: active; Author accepted graphical review and approved source freeze
and clean candidate preparation; physical text/browser qualification pending.
Started: 2026-09-08.
Coordinator: [PORT-014](../../../agon-extender/docs/tasks/PORT-014.md).

Author approved EMOS v0.7.0. Add Legacy-only EMOS VDPTEXT using the proven
private UART polling/RTS ownership, 1152000 baud and finite deadlines. Send
the coordinator's exact clear/position/text/flush/General Poll sequence;
require 80 01 A6 and a quiet interval. The ACK proves parser progress; the
browser-visible result is a separate hardware gate. Keep UART0/sysvars,
ordinary VDU routing, and the onboard console/clock unchanged. No mode switch
in the fixture; autoexec owns VDU 22 3.

1. [x] Implement bounded command and meaningful positive/negative checks.
2. [x] Run full configured/linked/runtime/host gates and same-build graphical
   ordinary/no-peer review before source freeze.
3. [ ] Consume the coordinator's accepted hardware/browser evidence.

Official reference contracts and exact release commits are recorded in PORT-014.

Draft agon-emos-v0.7.0-b2026-09-08-19-16-36Z passes full configured
qualification, linked UART/parallel and runtime checks, and all 74 host tests.
Same-build stock/EMOS ordinary and bad-SD smoke pass. The CLI no-peer text
diagnostic reports no reply from EDP and returns to MOS; the graphical
backend may instead report transmit timeout. Both are expected no-peer
outcomes. Review profiles are under the coordinator's ignored .emulator tree.

The preceding draft's shell checker reported a missing HELP marker. Complete
re-execution transcripts from that unchanged binary pass the strict comparator,
as does the fresh full build. Cause remains unestablished; no check was waived
and generic builder code is unchanged. Private logs retain the observation.

The Author's screenshot confirms the same draft identity, SD/CLOCK PASS,
expected transmit timeout and final MOS prompt. The Author explicitly approved
source freeze and clean candidate preparation on 2026-09-08. The coordinator
separately requires retained-parser UART acceptance and browser-visible text;
physical installation remains pending.
