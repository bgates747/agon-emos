# INTEG-008 — EMOS-owned visible-text diagnostic

Status: complete — coordinator hardware count and repeat runs passed.
Completed: 2026-09-08.
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
3. [x] Consume the coordinator's accepted hardware/browser evidence.
4. [x] Add the bounded resident `edu.text-probe` gateway for the coordinator's
   SD-loaded C sample, preserving Legacy routing and the original VDPTEXT
   command. Validate buffers/grammar before UART access, retain finite waits
   and release, and complete ordinary/no-peer emulator review before freeze.

Author-approved continuation: EMOS v0.1.7 is the deliberate early-development
numbering reset after historical v0.7.0, not a binary rename. The sample now
owns the banner and C decimal conversion. Core accepts a bounded text buffer
through existing API 0x51 and owns the entire transport/completion exchange;
the coordinator records the exact grammar, hardware and procedure identities.

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


The v0.1.7 b2026-09-08-20-34-02Z draft passes configured qualification,
linked/runtime gates and all 74 host tests. The coordinator's SD-loaded C
program exercises actual API 0x51 rejections and bounded no-peer return in the
emulated eZ80, plus count preview and same-build SD/CLOCK smoke. Graphical
review has been launched; no new commit or physical deployment is authorized
by these automated results. PORT-014 records the complete paired build IDs.


The Author requested 250 ms between visible count increments. The application
now sends each line separately; EMOS suppresses per-call success chatter on
its gateway path while retaining failure output and old VDPTEXT diagnostics.
The refreshed v0.1.7 b20-46-22Z draft again passes all 74 tests and complete
configured/linked/runtime gates. Coordinator's paced application passes actual
eZ80 gateway/no-peer, preview and SD/CLOCK checks. Fresh graphical pacing
review remains pending.


The Author confirmed the paced graphical result and requested deployment to
the mounted SD card. Reviewed implementation is unchanged; v0.1.7/r03 advance
to candidate in registry r34. The required clean-source freeze precedes new
candidate builds, automatic same-build review, then guarded MOS-only install
media. Physical counting and P4 deployment remain separate later steps. No
historical identity or result is relabelled.


## Completed — coordinator hardware evidence

PORT-014 run `PORT-014-2026-09-08-21-22-24Z` passes the SD-loaded application's EMOS gateway path,
11 exact retained-parser request/reply transactions at 1152000 baud, framing
and flow permission. The analyzer retained all 288M samples (12 seconds at
24 MHz), including 6.287879 seconds of final quiet; serial tail is 6.008467
seconds. The Author supplied the rendered banner/count screenshot and confirmed
SD/CLOCK, sample PASS and final MOS prompt for the captured run plus two
additional Agon-only resets. Only the first run is waveform-captured.

Installed EMOS is `agon-emos-v0.1.7-b2026-09-08-20-53-57Z` from clean `026ac46`.
The coordinating sample and P4 receiver are the clean r03 candidates from
`18024d8`. INTEG-008 is complete; the coordinator retains the authoritative
cross-component evidence at
`../agon-extender/hardware/designs/light2-harness-r03/tests/PORT-014-2026-09-08-21-22-24Z/`.
Historical r01/r02 limitations are unchanged. This proves bounded Legacy
qualification traffic; it does not activate Exclusive Compatible or alter
ordinary VDU routing. No EMOS source or artifact identity changed in closeout.
