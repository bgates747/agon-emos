# INTEG-019 — Minimal application network message admission

Add a small fixed endpoint to the corrected Eirene-based EMOS candidate.
The application owns message encoding/decoding and P4 owns sockets. EMOS retains
caller admission, UART ownership, safe copying/correlation and lifetime cleanup.

I19-PLAN-001 [x] Commit/push this scope before implementation. The Author's
verified MechAgon overnight driver contract permits bounded matched owner work;
Extender NET-003/INTEGRATION.md owns cross-component scope and deployment gates.
Start from exact corrected native Eirene16ec1f78, preserving all current features.

I19-ENDPOINT-001 [x] Negotiate and implement ext.net through existing0x51 gateway;
no new RST/API slot, game interpreter, CRC or substitute checksum. Static ordinary
application RAM, foreground enabled IRQs; no app pointer or ISR callback retained.
Use fixed endpoint/version/token/query validation and one bounded immutable reply.
Reuse current serializer, receive-buffer dispatch and app-exit/fault cleanup.

I19-CLIENT-001 [x] Keep the caller's codec/poll/timeout policy in the public
application include and document a plain C/assembly path. No MOSlet or resident
network stack; no ordinary display-route override. Preserve current controller,
SD/parallel, keyboard and async UART behavior.

I19-CHECK-001 [x] Test actual service C with the real P4 peer/app codec and fake
physical transport, including memory ranges, IRQ/caller exclusion, wrong/stale
reply, busy, lifecycle and fault/escape cleanup. Build via repository-root
make firmware-check with every mandatory native linked check, including divisor.
Measure complete ROM/static RAM against exact starting composition before flash.

I19-DEPLOY-001 [ ] Installation remains a later explicit matched component plan,
fresh central bench lease, full incoming backup and independent readback/restore
or selected live handoff. Normal startup unchanged; no automatic tests on reset.
No emulator, production promotion or broader API qualification is included.

## Software checkpoint

Actual service/P4 peer/public C client pipeline passes sanitizer checks; native
C gateway leaf compiles and preserves IX/IY. Root firmware-check passes all five
mandatory linked checks. Added1964B ROM,148B static RAM; unidentified ROM130123B,
free949B. This is not installed firmware or a physical timing claim. The endpoint
uses32-bit queries and bounded142B reply storage. Extender owns matched evidence
and a later fresh identified build/deployment plan.

## Author-requested resident reduction

I19-REDUCE-001 [ ] Preserve the checked1964B-addition candidate, never flash it.
The Author asks why the work is not in application space and explicitly allows
continuing the offload. Consult the consumer about its already-owned callback;
move mailbox/correlation/timeouts into the app. Resident retains minimal admitted
carrier forwarding and borrows the existing MOS reply callback, with no new
application pointer registry. Existing input lifetime is a synchronous immutable
application buffer. Measure actual native savings and repeat all mandatory gates
before selecting a new identified development revision.

### Reduced software evidence

The actual P4/service/app pipeline passes ASan/UBSan, including a reply delivered
before SUBMIT returns, previous-vector snapshot, expiry and explicit release.
Native C/router assembly and all five linked checks pass; the keyboard linked
checker now verifies the borrowed callback trampoline exactly. No old controller
code or UART behavior changes. Unidentified ROM129031B versus130123B for the first
network candidate:1092B saved. Static network state4B versus148B. Freeze as r04;
identified strings and like-for-like baseline overhead are measured separately.
