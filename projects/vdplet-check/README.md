# Finite VDPlet admission check

This console utility checks that EMOS forwards a real mainboard VDPlet discovery
reply to the registered MOS callback, keeps keyboard state unchanged for that
reply, and still delivers Escape. It performs no rendering benchmark, mode
switch, timer write, raw UART access or SD write. The caller saves the result
from application RAM before loading anything else.

It is bounded support for the approved v0.1.27 correction, tracked in
[INTEG-015](../../docs/tasks/INTEG-015.md), rather than a general VDPlet client or
production qualification suite. The installed mainboard VDP must support the
documented ten-byte `VDPL` discovery reply. The EMOS route must be Legacy and
the intended keyboard provider must already be admitted.

Build with the approved EMOS software identity and a fresh UTC build timestamp
in `PROBE_BUILD_ID`, using the configured `AGONDEV_TOOLCHAIN`. The default
identity is `UNVERSIONED-DO-NOT-DEPLOY`. The application prints its exact utility
build ID. Its identity is separate from the installed resident ROM build.

1. Record the binary hash and matching linker map; use the map's
   `_vd27_result` address, never an address from another build.
2. Explicitly LOAD and RUN the utility through the admitted CLI. Startup and
   reset must not launch it. It queries fresh entry mode information and stays
   in that mode throughout.
3. The utility drains the invoking Enter release before taking its keyboard
   baseline, installs its callback and sends only the discovery request. The
   callback copies the payload or latches Escape without libc, VDU or file work.
4. Send Escape after discovery. The utility bounds this wait, clears its vector
   and returns to MOS on either success or failure. The callback suppresses
   character publication while retaining normal key-map handling for real keys.
5. SAVE 64 bytes from the exact `_vd27_result` address, then enter the maintained
   fast-v2 listener if host retrieval is needed. Keep the caller batch finite
   and use new evidence names. Never overwrite the running caller batch.

| Offset | Bytes | Result |
| --- | --- | --- |
| 0 | 4 | `VD27` |
| 4 | 1 | Flags: fresh mode 1, valid discovery 2, unchanged key state 4, Escape 8, vector removed 16; PASS is 31 |
| 5 | 1 | Fresh entry mode |
| 6–8 | 3 | Key-event counts before discovery, after discovery and after Escape |
| 16 | 10 | Unchanged received discovery body |
| 32 | 16 | Key map before discovery |
| 48 | 16 | Key map after discovery |

This verifies finite callback admission and input handback on the recorded
combination. It does not qualify rendering, every reply kind, application module
lifecycle, controller latency or physical scanout. Raw-clock calibration uses a
separate host-owned console/SAVE procedure and does not alter this utility.
