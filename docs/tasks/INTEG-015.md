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
