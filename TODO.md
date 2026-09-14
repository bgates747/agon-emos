# EMOS TODO

## First priority — UART execution cost

- [ ] **INTEG-014 — Diagnose and reduce EMOS UART execution cost**
  - Status: Supervised run authorised; E02 builds and memory accounting passed; supervised pause before E03. Ordinary EMOS has 157 flash bytes free; bench has 1,001. No firmware implementation started.
  - Details: [INTEG-014](docs/tasks/INTEG-014.md). Cross-component qualification remains Extender PORT-008.


This is the repository's single authoritative list of unfinished EMOS work.
Cross-component Extender work remains in the `agon-extender` task list; generic
port-infrastructure work remains in `mos-agondev`.

INTEG-013 was accepted on 2026-09-13 UTC and removed from this unfinished list.
See [the dated record](research/devlog/2026-09-13.md) and
[SD service guide](projects/sdserve/README.md). The existing queue below is
preserved; no downstream implementation was started during SD delivery.

## Queued graphics timing reception

- [ ] **INTEG-012 — Private graphics benchmark completion reception**
  - Started: 2026-09-11
  - Status: Draft diagnostic v0.1.13; ROM and emulator gates pending.
  - Details: [INTEG-012](docs/tasks/INTEG-012.md)

## Ordinary ExCom console

- [ ] **INTEG-011 — Preserve displays during application-requested route switches**
  - Started: 2026-09-09
  - Status: Draft --keep-display command option for Extender paired graphics fixtures; retain EMOS coordinator and public mos_oscli ownership.
  - Details: [INTEG-011](docs/tasks/INTEG-011.md)

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
