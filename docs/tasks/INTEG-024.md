# INTEG-024 — Application-owned UART1 async owner binding

Implement only the minimal admitted IRQ/status/CTS retry glue for the selected
provisional application driver. Source/state/buffer/serializer live in app RAM;
existing synchronous net provider remains supported. No vanilla vector takeover.

## Scope and research

Parent cross-component plan: Extender docs/tasks/NET-004/EMOS-ASYNC-R01.md.
Baseline2c79c600 native UART-NM/guarded-RFC25 source, installed rawROM129108B,
1964B free. Reuse UART core823bb91 in application space. Read-only MOSv3.0.2
8336409351ee5314e02801a7b72a4f1bb5282519 and docs f9806bd3 before owner changes.
No new numeric MOS syscall; explicit opt-in provider via existing service ABI.

## Instructions and validation

1. [ ] Define/apply atomic attach/submit/service/progress/positive-detach contract;
   validate caller/IFF/spans/mode, keep UART1 RX/error/vector with EMOS, exclude
   every other UART1 writer/route/shared-pin operation while attached.
2. [ ] One bounded TX burst after RX, one shared LSR/error observation, GPIO CTS
   refusal disables TX IRQ; existing tick retries and foreground progress checks
   later TEMT. Retain first fault and borrow on refusal. No indefinite ISR wait.
3. [ ] Remove app hook atomically before storage reclamation; abnormal exit faults
   owned stream, never claims cancellation/completion. Old net behaviour preserved.
4. [ ] Add focused actual-source host tests and linked ownership/IRQ/ROM checks.
   Run repository-root native firmware-check retaining all mandatory validators.
   Finite15min normalPiFIFO, serial compile with owned children joined/released.
5. [ ] Report complete resident delta/remaining ROM plus exact private SDK/ROM/map
   build pins and rollback to Extender. Physical trial requires separately frozen
   matched consumer/source/kit/current tuple and normal fresh bench custody.

State: task-first; no implementation/build/install/physical pass yet. Current
MechAgon R02 is independently frozen and must not acquire this new driver.
