# PORT-008 cold-boot fixture

This directory owns the fixed-purpose ordinary eZ80 application used to prove
the first forward-only EMOS-to-EDP path. `fixture.json` vendors the accepted
106-byte VDU command and independent browser-frame oracle from `agon-extender`
commit `c03656c`. `build_fixture.py` deterministically generates an ADL program
that invokes the ordinary `RST.LIL 18h` surface; it contains no GPIO access,
EDU envelope, discovery operation, return parser, or Extender-only API.

Run `make port008-fixture` at the EMOS repository root. The reviewed artifact is
`build/P8VDU.BIN`; generated files are ignored and are never source authority.
The frozen VDU payload is 106 bytes. The complete executable is currently 118
bytes because its generated 12-byte entry wrapper loads HL/BC, invokes ordinary
`RST.LIL 18h`, and returns; the verifier locates and hashes the payload by linked
symbols rather than confusing the whole executable with the command stream.

After a fixed-purpose EMOS image has been built through `mos-agondev`, run
`make port008-linked-check`. That check disassembles the linked sender and
verifies the READY loops, Port C data write, high-to-low Port D clock sequence,
4096-byte bound, and exact General Poll bytes.

The later physical procedure uses these exact SD-card files:

1. `/emos.bin` — a separately identified PORT-008 EMOS firmware candidate.
2. `/P8VDU.BIN` — the identified artifact built here.
3. `/autoexec.txt` — exactly two nonblank lines, in this order:

   ```text
   EMOS MODE EXTENDED
   P8VDU.BIN
   ```

There is no timer-based startup guess. With both boards powered down, the
operator installs the reviewed harness and media; powers and qualifies the P4
candidate and HTTP browser first; then cold-boots the Agon. EMOS starts in
Legacy, the first autoexec line requests Exclusive Extended through the normal
mode transaction, and the second executes the ordinary fixture. On success the
fixture returns to MOS while Exclusive Extended remains committed. Because the
prototype has no reverse transport, evidence comes from the browser frame,
P4 transport metrics/logs, and any separately qualified logic capture—not from
a VDP response packet or canonical MOS sysvar update. Resetting the Agon returns
EMOS to its Legacy cold-boot default.
