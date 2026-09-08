# Accepted hardware milestone and evidence freeze

Accepted 2026-09-07 local / 2026-09-08 UTC. After the recorded three-cold-boot
pass and successful flash, the Author instructed: “yes correct and freeze.”

This accepts the bounded ordinary Legacy boot milestone and authorizes the
evidence commit and procedure correction. QUAL-002 is complete and removed
from the authoritative TODO. The observation limits and installation deviation
in the completed [run manifest](manifest.yaml) remain intact. This closeout
adds acceptance without changing the completed manifest or its hashed evidence.

The [r01 procedure](../../emos-ordinary-boot-r01.md) is preserved byte-for-byte
from candidate commit `e8fd3797e18f77ab2e35d8e66176098af1f668d9`.
The [corrected r02 procedure](../../README.md) uses lowercase `mos` and the
operator-confirmed relative payload name from the card root, retaining the
rename-before-flash guard. Registry r23 records that correction. The original
installation script and the operator's retry script remain separate evidence.
The completed run is not relabeled as an r02 run.

No firmware source, binary, build identity or diagnostic status changed. The
accepted runtime result applies to the exact candidate already installed;
the new combined r02 installation script has not itself been run on hardware.
No rebuild, reflash, hardware operation, release tag or broader qualification
claim accompanies this freeze. Firmware and corrected procedure remain at
candidate status. QUAL-001 retains broader hardware qualification, and no
UART1/P4 or Exclusive Compatible implementation is started by this acceptance.
