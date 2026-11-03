# First Eirene implementation tranche: functional result

Applications can now hand one buffer to MOS and calculate while interrupts send
it to the VDP. This Tom-derived implementation is476bytes of code and11bytes of
RAM. It leaves ordinary RST18 synchronous and changes no startup, reset, serial
or receive-parser source. The one-buffer interface is simpler than the previous
queue/copy prototype. We have proved its finite behavior in Flexe; we have not
yet measured its hardware performance or approved it for physical installation.

## Pinned code and artifacts

Repository: https://github.com/bgates747/eirene-mos, branch codex/uart0-async.

| Component | Full source commit |
|---|---|
|Pristine Eirene|86b694260e3be3b391284ce4aa8ecc758f7f34e9|
|Minimal sender / contract|b487e10b7d755ee94ac9516c85a9d828fd3ccd17|
|Finite fixture and tooling|4f6d058c905021b49d06719229b33ac88ff0a24e|

Use Eirene's AgonDev compiler/build, not the old ZDS II build. Firmware snapshots
and compiler logs are in build/tm001/{baseline,scalar003,otirx003}. The scalar
snapshot changes only UART0_ASYNC_SCALAR equ0 to1; both paths account once per
block. This is the lightly fixed Tom-derived scalar control, not the unchanged
attachment. Fixture build89b70a5ef4049425 uses a generated identity.h overlay;
all functional source otherwise matches its pinned commit.

| Image | Bytes | SHA256 |
|---|---:|---|
|Pristine|100660|2110243ec967b16426933f8ffbd411649e51eb47dea780a49f1644e5421e2a64|
|Scalar|101163|383167c7ae6220472378b366c63c81b9df5e3e63fe5d827233d4c2276c287965|
|OTIRX|101155|7db662439e33badd561cefe26e7a7a643d9f932da1031bce20d60217cd1e03af|

The complete OTIRX image adds495bytes, including the longer already-printed
variant label. Scalar code is8bytes larger. init.asm,reset.asm,handlers.asm,
interrupts.asm,serial.asm,vdp_protocol.asm and main.c match pristine source
byte-for-byte. Branding changes VERSION_VARIANT only, so it adds no startup call
or implicit transport activation. Existing CRLF formatting is retained.

Frozen Flexe frontend7e7d0e1bc89af72bde7ffbd6c06446aaefda4d1803f89eb1faf3167307ee1e01,
backend0866265bd6f3d73ad572f31dc1bb954f308ed9a5d857efa9414d37e1ade13374,
receiver-fixed VDP201ad8ab4af807c261cb92634286b2fbaedcf3879f140e820b60ab54cf707e2a.
These tests use actual ESP32 VDP firmware and precise UART interrupts; they are
not a native-host VDP simulation. This composition does not qualify stock VDP,
EMOS, physical electrical margins or another runtime cohort.

## Checks and result

| MOS / sender | JIT | Interpreter | Receiver payload per cell |
|---|---|---|---:|
|Pristine / ordinary RST18|PASS|PASS|8192bytes|
|Tom-derived scalar|PASS|PASS|8192bytes|
|Tom-derived OTIRX|PASS|PASS|8192bytes|

Async cases also passed boundary lengths1/15/16/17/255/256/257/1024, NULL callback,
zero-length and masked-context rejection, refusal of a second outstanding write,
a bounded C callback, immediate source overwrite after callback, and an early
GP response while a large transfer remained active. A temporary input-only
keyboard hook was installed successfully; actual key traffic was not injected.
Each case finished and returned to the ordinary MOS CLI, with no UART drops,
RX overruns or backend failed/unhandled/unregistered/unmapped firmware operations.

The receiver comparator's negative/positive controls passed before use.
Independent host analysis checked the complete8452-byte framed stream, every
one of the8192receiver-comparison commands and the1842boundary/no-callback no-op
bytes in each async cell. Total receiver payload verified:49152bytes. All four
async outgoing streams are identical after normalizing ONLY the leading boot
GP retries: scalar interpreter emitted one fewer four-byte boot probe. This
occurs before the banner and before sender activation; no payload discrepancy
was removed or normalized.

Model backpressure generated45..83 CTS edges and29..54 modem IRQs per async cell,
so the wakeup path was exercised during normal traffic. We did not inject a
specific CTS edge at each masking instruction or measure physical receiver
reserve. No CPU/throughput ranking is inferred from host wall time.

## Reproduction and custody

The self-contained fixture generator/build/run/analysis instructions are in
experiments/uart0-minimal/README.md. Its staged profiles and raw evidence are
preserved on Oryx at /home/smith/Agon/mystuff/eirene-mos/build/tm001/flexe002.
Pi mirror: build/tm001/flexe002. Raw evidence and matching binaries/maps archive:
build/tm001/functional-r02.tar.gz, SHA256
0e7622e151c1ee1556c0e5e3dc1013e5523ec7c2ea39f6e0d4017e66ee9378d0.
The JSON companion records independent byte/health checks and limitations.

First attempt flexe001 failed before any CPU execution: the staging script
copied a generated launcher without its executable permission. It was fixed,
fresh profiles were staged and all six r02 cells passed. Failed evidence remains.
Shared checkout201 was released before retry; successful checkout202 was released
with fresh whole-host idle evidence and no next request waiting. No lease remains.

## Next work

MI-005: controlled paced/useful-work and bulk performance measurements using the
same Eirene baseline, scalar and OTIRX implementations, with receiver checks.
Then TM-005: manual Console8 qualification and physical timing before trying real
applications. The existing balls demo is a later visible comparison. No SD card
or physical firmware was changed by this tranche. Automatic abandoned-session
cleanup, forced reset/crash misuse and additional queue/copy features were not
added. Correct explicit finish remains the caller's responsibility.
