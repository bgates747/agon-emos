# QUAL-001 — Qualify EMOS on physical Agon hardware

## State

- Status: Not started — external release gate
- Started: --
- Finished: --

## Intent

Execute the recoverable, Author-operated physical qualification defined in
`docs/port-203-hardware.md` against a committed EMOS candidate and preserve a
hash-bound capture. Emulator success does not satisfy this task.

## Dependencies and gates

1. The candidate source, generic `mos-agondev` revision, generated firmware,
   stock VDP, recovery MOS, hardware profile, and procedure must have reviewed
   identities before deployment.
2. Real EDP transport stages remain blocked until `agon-extender` supplies a
   qualified transport and wiring profile; those cells may be skipped only
   with explicit reasons.
3. The existing hardware and emulator human-approval policies remain in force.

## Completion criteria

1. Every applicable procedure stage passes on real hardware.
2. Every skipped stage has a bounded reason.
3. Raw artifacts and hashes validate with the maintained capture validator.
4. The Author accepts the evidence and release claim.
