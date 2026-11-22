# INTEG-019 — Author-approved flash path amendment

On November 22, 2026, Smith confirmed that the optional P4-to-eZ80 ZDI leads
are disconnected. His earlier wiring confirmation covered only the ordinary
UART/PARLIO harness. He directed proceeding with the standard flash utility
and selected the spare WROOM as the recovery route if an EMOS flash failure
leaves MOS unable to boot. This supersedes the mandatory P4 ZDI preflight and
fallback in the frozen [R02 contract](../INTEG-019.md). Preserve its other
qualification and production handback criteria.

The first preflight independently verified installation of the temporary P4
app, then stopped on ZDI identity `0000/00` where it expected `0007/AA`.
The receiver stopped before the eZ80 dump or any eZ80 write. The diagnostic
worker and children joined. Restore production P4 app0 using the pinned
extent, compare the full readback independently, and prove the production CLI
and services. Preserve the failed preflight evidence without replay.

Install and normally restore EMOS through the commissioned bench runner's
`FLASH mos` path. Preserve production P4 and mainboard VDPlet during the
candidate checks. If an unexpected fault occurs, retain the slot and follow
the bench stop rule. Use the selected WROOM contingency if a failed EMOS
flash leaves MOS unbootable. That contingency needs the actual WROOM setup,
the pinned production EMOS ROM, powered-off ZDI wiring, and full verification
under the [recovery procedure](../../../../agon-extender/docs/mos-recovery.md).
Its prior preparation was unqualified. No WROOM action is part of a normal
successful run. M304 remains held pending candidate qualification.
