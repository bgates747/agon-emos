# INTEG-003 — Exercise the installed EMOS UART1 driver

Status: caller and local checks complete; paired identity/freeze and hardware test pending.
Started: 2026-09-07.
Cross-component scope and bench gates: [PORT-009](../../../agon-extender/docs/tasks/PORT-009.md).

1. [x] Add one ordinary SD program using only installed EMOS MOS APIs to open
   UART1 at 115200 8N1 without flow control or RX interrupts, send the fixed
   test message once, allow transmit settling and close UART1. No direct GPIO,
   registers, vectors, routing or activation changes. The caller reports SENT,
   not P4 receipt, and ordinary diagnostics go to the onboard VDP.
2. [x] Compile with the existing AgonDev toolchain, inspect the linked API
   wrappers, and retain source/output hashes under the paired fixture build.
3. [x] Prepare keyboardless mode-3 autoexec with EMOS STATUS before/after. The
   initial EMOS STATUS also stops the script under stock MOS. Do not place a
   flash command on the card or rebuild the accepted EMOS firmware.
4. [ ] Record the paired hardware test after identity/freeze/bench approval.

The official API and implementation contracts are summarized in PORT-009's
bounded précis. This is a diagnostic client of EMOS's retained public UART
APIs, not a product route or a replacement UART driver. Closing UART1 does not
restore the GPIO mux; do not claim all Port C pads become passive afterward.
Hardware flow control, return data and Exclusive Compatible activation remain
later work. Existing candidate identity and three-boot evidence stay unchanged.

## Local validation and inherited wrapper defect

The ordinary `projects/uart-forward` caller compiles with warnings treated as
errors. The paired Extender build verifies the linked open/write/close wrapper
instructions and records sender source/output hashes. The message is exactly
18 bytes (`EMOS UART1 -> P4` plus CRLF); success reports SENT. Generated
autoexec remains in the local build output; no SD update has occurred.

AgonDev b67ab244's `src/lib/libmos/mos_uputc.src` converts EMOS carry-set success
to C return value zero, contrary to the installed header's comment. The caller
explicitly compensates and the build fails if the linked wrapper changes.
Review/remove this compensation when adopting a corrected upstream wrapper.
The actual installed library and compiler hashes are in the paired manifest;
the read-only AgonDev checkout was not patched. No EMOS firmware source or
candidate image changed. No physical UART result is claimed.
