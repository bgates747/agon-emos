# EMOS TODO

This is the repository's single authoritative list of unfinished EMOS work.
Cross-component Extender work remains in the `agon-extender` task list; generic
port-infrastructure work remains in `mos-agondev`.

## Current priority — browser keyboard

- [ ] **INTEG-009 — Receive stock keyboard packets over UART1**
  - Started: 2026-09-08 19:18 EDT
  - Finished: --
  - Status: Receiver/emulator checkpoints frozen; controlled P4 UART delivery and public-API effects pass on hardware with three Agon runs; physical browser typing, Enter and Backspace work; latency and apparent capture loss need coordinator-led diagnosis; broader integration remains.
  - Details: [INTEG-009](docs/tasks/INTEG-009.md)

## Other Extender integration

- [ ] **INTEG-002 — Implement the production forward-parallel data plane**
  - Started: 2026-09-01
  - Finished: --
  - Status: On hold while the UART keyboard increment takes priority.
  - Details: [INTEG-002](docs/tasks/INTEG-002.md)

## Hardware qualification

- [ ] **QUAL-001 — Qualify EMOS on physical Agon hardware**
  - Started: 2026-08-31 18:32 EDT
  - Finished: --
  - Details: [QUAL-001](docs/tasks/QUAL-001.md)
