# INPUT-001 — Resident controller read candidate

The Agon mode20 viewer needs controller state from P4 through EMOS. This
repository owns the candidate cache/gateway/ISR implementation. Extender owns
the assembled-product task, P4 peer, application and paired contract. No
firmware installation or production acceptance is implied by source/build work.

## Scope and state

The Author requests the Agon controller viewer and permits AgonDev C++ for that
app. The needed EMOS implementation is isolated behind `controller-candidate.mk`;
ordinary source selection excludes it. Maintained source stays here; the
existing reviewed mos-agondev wrapper builds a prepared copy. Foundation
adoption/toolchain migration and unrelated UART experiments remain separate.

1. [x] Implement `ext.controller` using the existing resident gateway, with
   application-range, mode, IRQ and healthy existing UART-owner validation.
2. [x] Store only resident packets; reject malformed/stale controller replies;
   copy state without waiting and acknowledge transitions only after READ.
3. [x] Clear ownership on application lifecycle/transport fault, preserve
   ordinary VDU, keyboard and SD ownership, and add focused native fault tests.
4. [x] Complete full-ROM build/mandatory linked guards and retain exact bytes:
   130,972bytes/100free, unversioned exploratory source; not qualified/released.
5. [ ] Review/identify the paired candidate before any firmware installation;
   physically validate source loss, UART delay, input, mode cleanup and coexistence.

The [paired development contract](../../../agon-extender/docs/protocols/controller-emos.md)
owns message layout and the private CI control namespace/type8E response.
It reuses existing RST08/getfunction gateway allocations; no global API number
is introduced. `emos_controller_wire.h` must match the P4 header byte-for-byte.
The app builder generates assembly bindings from canonical `mos_api.inc`.

## Implementation limits

READ is a resident copy; REQUEST uses the existing bounded UART serializer and
does not wait for a response. Nominal30-unit request/cache deadlines are not
calibrated wall clocks. The source-age/cache-age sum is conservatively limited
to250ms. Eight transitions are returned at a time; P4 retains its queue until
the subsequent acknowledgement. Boot-local caller tokens are not persistent
reset incarnations or authentication; physical reset/stale-byte boundaries
remain a qualification item. The new service is a candidate, not a public
production ABI. No callback or direct application UART access is supported.

## Research and evidence

Existing gateway and keyboard-owner contracts were read before implementation.
Official read-only MOSv3.0.2 and VDPv2.16.0 references remain unchanged; exact
commits and relevant API/font/mode documents are recorded in Extender INPUT-001.
The focused native harness runs the real service with explicit hardware fakes;
the paired native pipeline links it to the real P4 peer and Agon client.
These software checks do not qualify a ROM build or physical bench.

Build preparation in an isolated mos-agondev checkout required its ordinary
toolchain/interpreter/default-worktree input links. Those are ignored local
layout records, not product source edits or a reason to alter generated code.

The profile fails closed unless source/build identity are explicitly
`UNVERSIONED-DO-NOT-DEPLOY` and status is`experimental`. Its successful ROM SHA256
is`8feace3ad4a903e4d987933bad7824035b72994484ddb4e7f4b8e93ed5c8f6ee`.
Required UART divisor, keyboard, console, ABI and VDU linked guards pass. Exact
source/preparation/compiler/link records remain in ignored local evidence;
this is a dirty source build, not an accepted version or an installed image.

The final expiry correction uses the existing VBlank owner so foreground stalls
and low-byte clock wrap cannot revive a late reply. EMOS conservatively includes
the request round-trip in source age and neutralizes stale observations. Native
tests cover late replies across wrap and exclusion of parked UART ownership.
Native-width age arithmetic avoids AgonDev's16-bit`__smulu` helper outside the
reviewed runtime closure; no generic allow-list/runtime source was changed.
Only100 ROM bytes remain, so later capability growth needs a separate size plan.


Author agrees experimental EMOS v0.1.25, indexed-hdmi-r10 and
controller-viewer-r01 identities and directs proceeding with deployment.
Preserve mainboard VDP/startup; leave the viewer stopped for Author invocation.
This supersedes the pending identity gate above, not physical acceptance.


Identified deployment: P4 indexed-hdmi-r10, EMOSv0.1.25 and viewer-r01
installation files are independently verified. EMOS ROM is131010bytes/62free;
complete installed ROM readback matches the selected padded image. Mainboard
VDP and startup are unchanged. Viewer loaded, never RUN. Legacy CLI/input
healthy, listener closed. Oryx sender stopped at HTTP400 IPv4-required admission;
IDF intentionally accepts IPv4 through a dual-stack IPv6 listener. The P4
handler's sockaddr_in-only assumption is corrected in prepared source;12
focused tests and an actual P4 compile action pass. A new identified P4 build
and installation remain gated on its revision agreement; no EMOS or mainboard
reflash is needed. Physical controller/visual acceptance remains pending.
