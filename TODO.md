# EMOS TODO

This is the repository's single authoritative list of unfinished EMOS work.
Cross-component Extender work remains in the `agon-extender` task list; generic
port-infrastructure work remains in `mos-agondev`.

## Current priority — ordinary ExCom console

- [ ] **INTEG-010 — Ordinary ExCom console over UART1**
  - Started: 2026-09-09
  - Status: Software and Author mainboard visual checks pass; candidate preparation from frozen source. Physical ExCom proof remains open under Extender PORT-008.
  - Details: [INTEG-010](docs/tasks/INTEG-010.md)

## Established keyboard integration

- [ ] **INTEG-009 — Receive stock keyboard packets over UART1**
  - Started: 2026-09-08 19:18 EDT
  - Finished: --
  - Status: Controlled P4 UART/public-API hardware proof and basic browser typing pass. Native USB ordinary CLI, gameplay and bounded reconnect/source-return checks pass hardware under PORT-015. Wider keyboard parity and browser input are deferred; fix regressions blocking ExCom only.
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
