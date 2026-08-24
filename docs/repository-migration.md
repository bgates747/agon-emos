# EMOS repository migration

## Current authority

`agon-emos` is the sole maintained source and task authority for Extender MOS.
The repository split preserves the accepted implementation while removing
EMOS product history from the repositories that now have narrower roles.

The reconstructed lineage is:

1. official Agon Platform MOS v3.0.2 at `8336409`;
2. the generic oversized-VDP-packet correction at `a4fae76`, shared with the
   upstream-oriented `agon-mos` fork; and
3. the provisional EMOS v1 source at `0e1541c`.

The source tree at step 3 is byte-identical to the formerly published
`agon-mos` `dev/emos` milestone `a8dc891`. The product tooling, tests, research,
tasks, and qualification evidence were recovered from the formerly mixed
`mos-agondev` history ending at `a695599` and consolidated here by role.

Those former commit and branch names remain valid provenance in dated research
records. They are not current repository or branch authorities.

## Reproducibility result

The reconstructed source, using the explicit product profile in
`port/mos-agondev.mk` and the separated generic `mos-agondev` pipeline,
reproduces the previously approved candidate exactly:

1. binary size: 113,053 bytes;
2. SHA-256: `ae0432a84be4be2261095d449627af876c9662d40a08ac334981e12d6dda339f`;
3. EMOS host tests: 46 passed;
4. generic port tests: 105 passed, plus 11 restricted-runtime tests;
5. emulator boot, shell parity, linked VDP regression, and MOS contract gates:
   passed; and
6. EMOS linked ABI, VDU dispatch, and provider-module gates: passed.

Physical Agon qualification remains an external release gate under
`QUAL-001`. Repository migration does not convert emulator evidence into a
hardware claim.

## Superseded locations

1. `agon-mos` no longer owns an EMOS branch, implementation, or task queue.
2. `mos-agondev` no longer owns EMOS source lists, product commands, product
   tests, product documentation, or hardware-qualification records.
3. `agon-extender` owns cross-component requirements but not the EMOS
   implementation task queue.

Verified recovery bundles made before ref rewriting are retained outside the
repositories until the Author accepts the migration. They are recovery media,
not project history or a continuing source of authority.
