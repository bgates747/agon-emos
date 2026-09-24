# EMOS TODO

- [ ] **[AUDIT-008](docs/tasks/AUDIT-008.md)** — Retire cancelled provider machinery and validate minimal foreground /emos dispatch; deployed with bounded physical checks passing; broader acceptance pending.


## First priority — UART execution cost

- [ ] **[INTEG-014 — Diagnose and reduce EMOS UART execution cost](docs/tasks/INTEG-014.md)**
  - Status: E07P bulk parity complete in both profiles; exact original bench restored and hardware voice acknowledged. See docs/tasks/INTEG-014/E07P-results/README.md. E08 correctness consolidation complete; fresh linked/host and live service checks pass, unchanged-image physical evidence reused. E09 attempted: mainboard timeout and incomplete 550/624-interval retry; partial gains recorded, no full pass. Await Author disposition. Short-command setup remains documented; no experimental push.
  - Details: [INTEG-014](docs/tasks/INTEG-014.md). Cross-component qualification remains Extender PORT-008.

## Current BENCH-001 component work

- [ ] **[BENCH-001 — Resident telemetry transport](docs/tasks/BENCH-001.md)**
  - Status: Scope registered; implementation and validation underway.
  - Details: [BENCH-001](docs/tasks/BENCH-001.md)


This is the repository's single authoritative list of unfinished EMOS work.
Cross-component Extender work remains in the `agon-extender` task list; generic
port-infrastructure work remains in `mos-agondev`.

INTEG-013 was accepted on 2026-09-13 UTC and removed from this unfinished list.
See [the dated record](research/devlog/2026-09-13.md) and
[SD service guide](projects/sdserve/README.md). The existing queue below is
preserved; no downstream implementation was started during SD delivery.

## Queued graphics timing reception

- [ ] **[INTEG-012 — Private graphics benchmark completion reception](docs/tasks/INTEG-012.md)**
  - Started: 2026-09-11
  - Status: Private diagnostic callback implemented and exercised by later paired hardware timing; production generalized callbacks are not qualified.
  - Details: [INTEG-012](docs/tasks/INTEG-012.md)

## Ordinary ExCom console

- [ ] **[INTEG-011 — Preserve displays during application-requested route switches](docs/tasks/INTEG-011.md)**
  - Started: 2026-09-09
  - Status: Implemented --keep-display option used by accepted paired graphics hardware review; retain EMOS coordinator and public mos_oscli ownership, with wider QUAL-003 coverage distinct.
  - Details: [INTEG-011](docs/tasks/INTEG-011.md)

- [ ] **[INTEG-010 — Ordinary ExCom console over UART1](docs/tasks/INTEG-010.md)**
  - Started: 2026-09-09
  - Status: Ordinary ExCom physical milestone accepted under Extender PORT-008; wider lifecycle/VDU compatibility remains open.
  - Details: [INTEG-010](docs/tasks/INTEG-010.md)

## Established keyboard integration

- [ ] **[INTEG-009 — Receive stock keyboard packets over UART1](docs/tasks/INTEG-009.md)**
  - Started: 2026-09-08 19:18 EDT
  - Finished: --
  - Status: Controlled P4 UART/public-API hardware proof and basic browser typing pass. Native USB ordinary CLI, gameplay and bounded reconnect/source-return checks pass hardware under PORT-015. Wider keyboard parity remains open. Browser input was reintroduced under Extender REMOTE-001; retain its separate platform/gameplay limits.
  - Details: [INTEG-009](docs/tasks/INTEG-009.md)

## Other Extender integration

- [ ] **[INTEG-002 — Implement the production forward-parallel data plane](docs/tasks/INTEG-002.md)**
  - Started: 2026-09-01
  - Finished: --
  - Status: On hold while the UART keyboard increment takes priority.
  - Details: [INTEG-002](docs/tasks/INTEG-002.md)

## Hardware qualification

- [ ] **[QUAL-001 — Qualify EMOS on physical Agon hardware](docs/tasks/QUAL-001.md)**
  - Started: 2026-08-31 18:32 EDT
  - Finished: --
  - Details: [QUAL-001](docs/tasks/QUAL-001.md)
