# EMOS documentation

Repository ownership and reconstruction provenance are recorded in
`../OWNERSHIP.md` and `repository-migration.md`.

1. `emos-v1-contract.md` defines the provisional EMOS Core, module/service,
   application-safety, dispatcher, mode, and ownership contract.
2. `emos-v1-qualification.md` preserves the superseded first candidate's
   automated evidence and its limits; it is not a current qualification.
3. `port-200-qualification.md` preserves the accepted emulator, ABI, parity,
   and reproducibility qualification of the first candidate.
4. `port-203-hardware.md` defines the outstanding physical gate.
5. `prior-art-rainbow-mos.md` records the prior-art review.
6. `tasks/` contains detailed unfinished-work authorities indexed by
   `TODO.md`.
7. [Ordinary boot test sheet](qualification/minimal-boot/README.md),
   `emos-ordinary-boot-r02`, is the corrected candidate procedure for automatic
   Legacy boot and MOS-only installation under QUAL-002. The
   [ordinary hardware smoke passed on three cold boots](qualification/minimal-boot/runs/QUAL-002-2026-09-08-01-23-44Z/hardware-result.md).
   Revision r02 corrects the case-sensitive updater argument and retains the
   rename guard; original r01 evidence is archived. The Author accepted and
   froze the bounded milestone. Artifact status remains candidate.

Repository ownership is defined by the root `OWNERSHIP.md`. Historical
reasoning and chronological implementation records are under `research/`.
