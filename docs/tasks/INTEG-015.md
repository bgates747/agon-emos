# INTEG-015 — Preserve Pingo render-completion callbacks in EMOS

Before this correction, EMOS rejected Pingo's ten-byte render completion on mainboard UART0:
keyboard selection excludes it when Extender input is active, and ordinary-key
validation rejects its fourth byte in every source selection. Correct that
compatibility break with bounded assembly dispatch, preserving source isolation
for real keys. This is not a new generalized callback API or a Pingo wire change.

## Contract — Author assigned 2026-10-07

I15-01 [x] Review the official callback ABI and exact sender/application path.
I15-02 [x] Admit only exact ten-byte `P3DR` KEY completions from mainboard while
EMOS commits Legacy/mainboard display ownership. Invoke the registered vector
with the original payload in DE, or drop silently if none. Do not publish any
keyboard sysvars, map, held-state or event count, even if the callback edits its
payload. Preserve ordinary four-byte key validation/source gating and UART1.
I15-03 [x] Build an identified draft through EMOS's root wrapper, run required
linked checks and execute the actual parser/dispatcher in the existing eZ80 CPU
harness. Test all length boundaries, absent callback, malformed signature,
Extender/mainboard selection, inactive display source, callback mutation,
interleaved genuine keys and frame recovery. Preserve a failing old-image control.
I15-04 [x] After bench handover, identify/preserve installed EMOS and startup,
install/verify only EMOS, confirm Extender input and exercise the existing fsim
review with unchanged Pingo VDP/application. Record actual completion/input and
return-to-MOS observations; leave fsim scene acceptance with its owner/Author.
Do not replay uncertain WebDAV jobs, replace P4/mainboard VDP, or reset another
owner's active work. Author explicitly released the bench for installation/testing on 2026-10-07.
First recheck live readiness and preserve the installed state. No commit/push or production-promotion claim in this tranche.

## Explicit VDPlet reply admission — Author approved 2026-10-25

The Author approved implementing, testing and flashing the bounded EMOS fix in
the fsim deployment chat. The actual human response was independently read;
the mailbox handoff alone is not the authorization. This extends the exact
signature set to `P3DR` and `VDPL`, both ten bytes, with the existing committed
mainboard-display guard and callback-only semantics. Preserve the installed
optional controller composition, P4, mainboard VDP, input selection and startup.
No wider ABI, fixture invocation or production promotion is implied.

I15-05 [x] Extend the maintained assembly and exact linked instruction guard
for the two signatures. Leave four-byte key filtering, UART1 and callback-only
return unchanged; introduce no checksum, wire fields or resident state.
I15-06 [x] Extend the actual linked CPU regression for both signatures and
retain v26 as control: it accepts `P3DR` and rejects `VDPL`. Exercise lengths
0–255/stale tails, all magic positions, hybrid signatures, both display owners,
all input sources, missing/mutating callbacks, ordinary key interleaving, payload
pointer/stack, IRQ state and parser recovery. Run existing keyboard, oversized
packet and controller regressions plus every profile-owned wrapper check.
I15-07 [x] Freeze an identified controller-enabled candidate after version
approval, measure ROM headroom, stage it on the correct host-mounted card, and
coordinate card return and exclusive bench handoff. Preserve the actual old ROM
and startup; issue FLASH once and independently read back the whole 128 KiB.
Leave the four wireframe fixtures stopped for their owning agent/Author.
I15-08 [x] Record installed identities, evidence and exact terminal state;
refresh the maintained contract and send fsim the bounded result and remaining
physical client/visual qualification limits. Preserve historical evidence below.
I15-09 [x] Verify real mainboard VDPL discovery through a finite console-only
callback check, with unchanged keyboard counters/map for the reply, actual
Extender Escape delivery, vector removal and CLI handback. The support utility
stays in its entry mode, queries fresh mode data, bounds waits, and saves results
only through an explicit finite caller batch. No wireframe benchmark is run.
I15-10 [x] Record a bounded raw-clock versus host-monotonic sanity check in
actual mode20 to distinguish nominal100/120 units/s. Preserve original mode and
route, allow at least one second after actual switches, retain raw samples and
host endpoint uncertainty, and do not change MOS's clock. Report rate and limits
to the consumer owner before performance claims.

### Bounded research for the extension

1. The official documentation and release baselines below remain unchanged:
   `mos_setkbvector` supplies DE=payload inside UART0 IRQ and requires callback
   removal before application exit. No payload length is passed to the callback.
2. VDPlet `c4ed7574787768251c882954edb10231b2782cbf`,
   `../agon-vdplet/docs/API-reference.md` and `src/vdplet_runtime.h`, defines
   mainboard UART0 KEY/0x81 length10 as `VDPL`, protocol:u8, kind:u8, token:u16,
   value:u16. The application validates these fields; EMOS passes them unchanged.
   This explicit second signature is not arbitrary extended-key forwarding.
3. Installed v26/source `11b7a5c1c422519451f3738d138c329aa838ae86`
   has 130786 raw ROM bytes/286 free. The proposed dispatch adds sixteen
   instruction bytes, no RAM; actual linked size remains to be measured.
4. Existing regression's historical old-image control rejects `P3DR`; that
   assumption is wrong for v26. Keep the old test evidence and use the actual
   matched v26 image/symbols for this extension's negative `VDPL` control.

### Current gate

The Author separately approved `agon-emos-v0.1.27` naming and confirmed the card
returned to Agon with reset required; both actual human responses were read.
The approved EMOS-only installation and finite compatibility/clock checks are
complete. The owning peer has exclusive bench/transport ownership again for its
timing-only client update. No wireframe benchmark was invoked by Extender.

Named `agon-emos-v0.1.27-b2026-10-26-01-06-54Z` passes the same mandatory
wrapper/linked guards, 1594 actual parser CPU cases and the linked controller
overlap/Escape regression. Raw ROM is 130802 bytes/270 free, SHA-256
`6a1c9b4ec244342c80618f2e23ef326b61d95910c5bfe3e85e539c6889c2a95c`;
the padded 128 KiB SHA-256 is
`84da2c5ac3cebb1c470bab201f797a936296a6bfc33b6ebdbcf5d1dd3a1d1d79`.
This is an experimental build of the exact archived uncommitted source patch,
not a clean-input qualification or production promotion. The retained archive,
source hash inventory, patch and root-wrapper provenance identify its inputs.
Only `src/vdp_protocol.asm` differs in resident source from installed v26.

The host-mounted Extender card contains verified `/extender/install/em-v027.bin`
plus finite `vd27-pre.txt`/`vd27-post.txt` ROM-save/listener batches and a build
receipt. Startup and the existing listener remain byte-identical; the four
consumer fixtures are untouched. Writes were flushed and the card was left
mounted. The Author subsequently returned it to Agon; installed results follow.

The unversioned controller-profile root-wrapper build and all required linked
checks pass. Its raw image is 130764 bytes/308 free; its shorter unversioned
identity makes that size unsuitable for predicting named-build headroom without
accounting for the identity difference. Dispatch itself grows exactly 16 bytes.
Expanded actual linked parser tests pass 1594 cases against the exact v26 image;
the linked controller pending-reply/READ/Escape test also passes. Existing
sanitized keyboard tests in three configurations, all three oversized-packet
tests and the controller lifecycle host test pass. These are offline checks,
not physical VDPlet-client or UART timing qualification.

### Verified installation and finite physical result — 2026-10-25

One ordinary Agon reset restored card access and fresh admitted input. Before
replacement, full 128 KiB ROM matched v26 padded SHA-256
`cc6d6d589a4c48b6713180f1d7bcb7966c735969c5fef349477c554fd8733a64`.
The host retained it and independently read the staged candidate, startup and
listener. FLASH was issued once; after its automatic reboot, all 128 KiB match
the named v27 image above, including padding. P4 and mainboard VDP were neither
flashed nor reset separately; startup and listener remain byte-identical.

The finite console utility in `projects/vdplet-check` was built as EMOS software
support `agon-emos-v0.1.27-b2026-10-26-02-12-16Z`, 8480 bytes, SHA-256
`e4d7834aaa8e74da3cb481073723e03314bd9dcbc11dc6df80f7a554ea30871c`.
Its actual compiled callback first passed 14 linked-parser CPU checks. The
utility stays in its entry mode; the host separately owns mode20 preselection
and restoration. Both finite physical runs pass:

| Observation | Original mode0 | Mode20 |
| --- | --- | --- |
| Real discovery body | `5644504c010100000f00` | Same |
| Key-event count before / after discovery | 165 / 165 | 101 / 101 |
| Key map changed by reply | No | No |
| Extender Escape observed / vector removed | Yes / yes | Yes / yes |
| Result flags / explicit return to caller | 31 / pass | 31 / pass |

The body is `VDPL`, protocol1, IDENT, token0 and capabilities0x000f. Results use
`/agents/extender/results/vd27probe1.bin` and `vd27probe2.bin`; matching source,
map and exact result address are retained. This verifies discovery callback
admission and keyboard coexistence, not all reply kinds, module lifecycle,
rendering, controller latency or visual correctness. P3DR preservation is
covered by the linked code; the earlier physical Pingo evidence is unchanged.

The separate console/SAVE clock check records mode20 raw values 56268, 57858
and 59434. The first-to-last delta is 3166 units; host midpoint elapsed is about
26.475 s, giving 119.584 units/s. Final-Enter / newly-online-listener bounds and
two-unit quantisation give a conservative 114.151–125.552 units/s interval.
Both adjacent intervals also exclude100 and include120. This distinguishes
nominal120 from100; it is not an oscillator-frequency calibration. Each host
endpoint window is about1.24 s. MOS's clock is unchanged. The consumer owner
will declare nominal120 in its updated configuration and preserve raw ticks.

Fresh mode information confirmed restoration to the original Legacy mode0;
the host allowed over one second after actual mode confirmation at switches.
Keyboard input is admitted and neutral. The fast-v2 listener is intentionally
online/idle for the consumer's coordinated upload; all host locks/workers are
closed. The owner received explicit transport handback and measured-clock
evidence. No wireframe benchmark was run. A host receipt-name collision after
the first completed check was reconciled from its retained result/journal;
neither FLASH nor the check was replayed.

Installation/probe/calibration/terminal receipts and full images are retained in
the Extender ignored `agents/vdpl-admission` silo. Source remains uncommitted
and experimental; production selection is unchanged. Author acceptance and
broader consumer/visual qualification remain separate from these bounded passes.

## Bounded research

1. Official [mos_setkbvector](../../../../agon-docs/docs/mos/API.md), docs commit
   `f9806bd3cbff6ed5d1c08bef1d51fed11764b86b`: callback runs inside UART0 IRQ,
   DE holds the full payload; the application clears its vector before exit.
2. Official MOS v3.0.2, commit `8336409351ee5314e02801a7b72a4f1bb5282519`,
   [VDP parser](../../../../agon-mos/src/vdp_protocol.asm) invokes the callback
   before stock key effects. Both official checkouts remain read-only. The
   Pingo app neutralizes its synthetic event before stock keyboard processing.
3. The owned [Pingo bridge](../../../agon-vdp-pingo2/video/pingo2_bridge.h) sends
   KEY/0x81; [command owner](../../../agon-vdp-pingo2/video/pingo2_commands.c)
   emits `P3DR`, version1, kind1, u16 token, u16 sequence (ten bytes).
   Owner HEAD `11d2ce686df01bc5527af1f2c70d1db1f54f3dfb`; no changes here.
4. The [fsim callback](../../../agon-fsim/utilities/pingo2-linux-candidate/review/callback.asm)
   recognizes the four-byte signature, copies ten bytes then zeros the first
   four. The [handoff](../../../agon-fsim/docs/tasks/PINGO-029.md) reports clean
   app return at the menu, consistent with completion timeout but not yet a
   captured physical proof. Renderer rejection/deadline remain possible too.
5. Maintained [parser](../../src/vdp_protocol.asm) already has a callback-only
   branch for private graphics telemetry. Reuse that terminal dispatch after
   explicit length/signature/mainboard-display checks. Do not bypass the C key
   owner's selection/validation for ordinary keys or add polling/timeout work.

The completion body is passed unchanged; the app owns version/kind/token/sequence
interpretation. The exact length and signature distinguish this compatibility
record from keys; unknown extended KEY records remain ignored. Exact four-byte
length now gates ordinary mainboard keys so stale parser-buffer tails cannot
be reused as input. This is deliberately narrower than arbitrary extended
callback forwarding, since the ABI supplies no payload length to the app.

## Evidence and limits

Candidate `agon-emos-v0.1.24-b2026-10-07-23-11-33Z` compiles through the root
wrapper with all required UART divisor, parallel, keyboard, console, ABI and
VDU guards passing. Image 128622 bytes; SHA-256
`44d217b555f68a7d030bc0a89a2fa1d27f0fb537576f2925e90ef055b0a44d02`.
Actual linked CPU regression passes 796 cases, including an old-image rejection
control for each keyboard source, lengths 0–255, unchanged held/keymap/sysvar
state, callback mutation, real-key interleaving and malformed-frame recovery.
Existing sanitized keyboard tests pass in ordinary and telemetry configurations;
all three existing oversized-packet checks pass. The bounded physical callback/input
check below also passes; Author acceptance and fsim visual qualification remain pending.

Build migration note: the current host library differs from the builder's pinned
runtime archive. Recovered the exact pinned archive
`5de7878342ab6780593fbf8d54d8005960e2471bd0e34c0e307fbcfe42fef28a` and its
headers from retained build inputs, paired with native ARM host tools in an
isolated concrete toolchain tree. All builder policies/checks are unchanged;
unrelated shared-builder work and official toolchains remain untouched.
No full-system emulator profile or Pingo application/VDP was modified. Machine-local build receipts belong in the Extender ignored
`agents/pingo-callback` silo; meaningful outcomes are summarized here.


## Repeating the linked regression

For the v27 extension, the current harness requires the exact v26 image as its
old control: `P3DR` admitted, `VDPL` rejected. The historical v23 control below
belongs to the original correction's retained evidence, not the current test.

Use the repository-root firmware wrapper to produce the candidate. Preserve
`MOS.bin` plus `nm.txt` (the matching ELF's `ez80-none-elf-nm -n` output) for both
a pre-correction image and the candidate. Then run:

```sh
cargo run --locked --release --manifest-path tests/uart_put_cpu/Cargo.toml \
  --bin vdp_callback -- /path/to/old-image /path/to/candidate
```

The existing pinned `ez80` CPU dependency executes the full UART0 parser,
compiled C key owner and assembly effects. Only the application's callback and
hardware-port boundary are modeled. It does not assert real UART timing,
VDP rendering, full interrupt entry/exit behavior or physical firmware health.
The linked guard separately checks the callback bridge's exact instruction
shape. Firmware grows by 48 bytes versus the retained v0.1.23 build (build
identities differ); the new dispatcher instructions occupy 48 bytes.


## Physical result — 2026-10-07

The Author released the bench specifically for this correction. The host retained
actual installed ROM and original startup, staged and read back the candidate,
then invoked MOS FLASH once. After automatic reboot, the entire 128 KiB ROM
readback equals the candidate plus erased padding; SHA-256
`6f83bc21558ea7dcb665b60c18aa3a5c3030367f901e3e32ccbbf99eaf7f4019`.
Extender input re-admits with a fresh epoch and neutral state. Startup is unchanged.
P4 and the mainboard Pingo memory-diagnostic VDP remain unchanged.

The existing `p2review.bin` matches its frozen SHA-256
`ffa940c6cc978df6fc20dd51bc49cec5c963b3d85dfa10a072bf01df86822fe5`.
A finite MOS batch runs it, saves its actual post-exit counters, then enters the
foreground listener for retrieval. The host waits for the initial terrain,
selects the cube and Lara scenes, rotates each, then presses Escape. No synthetic
completion, framebuffer capture or application modification is used.

| Observation | Physical result |
| --- | --- |
| Valid render completions | 5 |
| Stale completions | 0 |
| Application failure code | 0 |
| Key FIFO overflow | 0; head and tail both 9 |
| Key down / up observed by app | 5 / 4; Escape release follows application exit |
| Exit and input handback | Normal return; Legacy MOS prompt, Extender input ready/neutral |
| Original startup | Unchanged by full readback |

The run lasted 57.415 s including deliberate dwell times, not a performance
benchmark. Scene image correctness remains for fsim/Author review. The listener
and finite batch have exited; no host test, capture, transfer or reset remains.
Support and evidence files use `/extender/install` and `/agents/extender/results`.
No replay or removal of prior uncertain fsim WebDAV transactions occurred.

Local receipts are `deployment.json`, `physical-review.json`, the saved 167-byte
application state and `handback.json` in Extender's ignored `agents/pingo-callback`.
The actual prepatch physical ROM is separately retained, SHA-256
`341cab32e2ba0cc49d8327ec91fa0673d69e960836b5da18c14c37b07cc434e0`.
It is not the byte-identical old linked image used for the CPU negative control;
those are distinct evidence baselines. The installed correction remains a draft,
not a production promotion. No commit/push was requested for this tranche.
