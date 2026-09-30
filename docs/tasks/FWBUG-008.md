# FWBUG-008 — Correct raw SD API write dispatch

## Executive summary

Official MOS 3.0.2 and current EMOS dispatch public raw-SD API `0x73` through
`_SD_readBlocks_API`, so a valid write request reads card bytes into the caller
buffer instead of writing the supplied bytes. AUDIT-010 authorizes a one-call
EMOS correction, with source and final-linked-image guards and reuse of the
existing `mos-tests` independent raw-image oracle. Physical sector mutation
remains a separate explicit authorization gate.

## State

- Status: Physically validated under Extender `A10-RP04`; awaiting Author
  acceptance and disposition.
- Started: 2026-09-29.
- Owner: EMOS `src/mos_api.asm`; `mos-tests` retains the runtime fixture.
- Dependencies: official MOS API contract, current ordinary EMOS build profile,
  existing `sd_writeblocks.rst08.001` and `SD_writeBlocks.c.001` controls.

## Authority and boundary

The official API documentation at `agon-docs` commit
`f9806bd3cbff6ed5d1c08bef1d51fed11764b86b` defines selector `0x73` as writing
`BC` blocks from the buffer at `DE`, using the sector/unlock structure at `HL`,
and returning status `0`, `1`, or `2` in `A`. Official MOS tag `v3.0.2`, commit
`8336409351ee5314e02801a7b72a4f1bb5282519`, contains the inherited wrong call.

This repair changes only the final C-driver call in `sd_api_writeblocks`.
Address normalization, push/pop order, public selector, parameters, status
return, raw driver implementation, FatFS, `sdserve`, and other APIs are outside
scope. The official reference checkouts remain read-only.

## Work and gates

F8-01 [x] Confirm the official API parameters/status contract and compare the
official and current EMOS wrappers. Record exact source identities and preserve
the upstream-shaped wrapper.

F8-02 [x] Add a source regression that distinguishes the read and write wrapper
targets, plus a final-image guard that decodes their linked `CALL` targets.
Demonstrate that the new guard rejects the inherited dispatch before correction.

F8-03 [x] Change only `sd_api_writeblocks` to call `_SD_writeBlocks_API`, with a
local inherited-defect comment and no official-reference edit.

F8-04 [x] Run the focused source tests, complete EMOS repository suite and
ordinary `firmware-check`, retaining exact candidate identity, image hashes and
linked call-target evidence.

F8-05 [x] Reuse `mos-tests` `sd_writeblocks.rst08.001` and
`SD_writeBlocks.c.001` against fresh raw images. Retain independent sector-two
readback and distinguish API status from actual media effect. Emulator evidence
does not replace physical evidence.

F8-06 [x] Pause for Author review before commit or physical execution. The
Author approved the candidate/tool selection and directed the commit-pinned
hardware run. Publication remains outside this gate.

F8-07 [x] Only after a separately reviewed procedure and explicit authorization,
run the physical destructive-sector test on controlled disposable media with
preimage, write, independent readback and restoration evidence. The authorized
run passed; this does not itself record Author acceptance.

## Candidate evidence

The uncommitted candidate is based on EMOS commit
`8f29bf811e4b989117e437a8e10afe2adbc77be7`. The candidate-specific prepared
tree records that commit plus tracked changes. `make firmware-check` completed
through the repository wrapper with actual-step provenance under the ignored
`build/fwbug-008/provenance/` directory. Its linked-image verifier decoded the
two wrapper calls and accepted only `_SD_readBlocks_API` for read and
`_SD_writeBlocks_API` for write.

The retained candidate files under ignored `build/fwbug-008/candidate/` are:

| File | SHA-256 |
| --- | --- |
| `MOS.bin` | `fa5fd6a4ca71670acd1e9a5b338e6b2d00392dde9d21608d1b15ccbf05269174` |
| `MOS.map` | `c17efd8c3622a2bb06927842b3ed63d290d3c32a26c82456741067ed095e4e90` |
| `MOS.elf` | `1dbb596efdb43ab95f0091ea9889b7f7776244832358b7f4c1a3e26594eabb31` |

The six focused ABI tests pass. The complete repository suite reports 126 of
134 tests passing. All eight failures reproduce unchanged from clean EMOS
`8f29bf8` against the same older default prepared tree: two prepared-source
manifest/assembly-set failures and six missing project-source subtests. They are
pre-existing prepared-input inconsistency, not candidate regressions; the fresh
candidate-specific prepared tree and product build pass.

The `mos-tests` driver originally rejected every non-stock MOS hash even when
the caller supplied an explicit profile and matching `qualification-pins.json`.
The uncommitted harness prerequisite moves the stock MOS/map assertions into
the default-profile branch; explicit profiles remain fail-closed against all
four declared hashes. No fixture behavior or oracle changed. The pinned stock
profile's separate C `SD_writeBlocks` control still passes both seeds after the
harness correction.

The two selected runtime functions pass four of four cases (two independent
seeds each), with no infrastructure errors. Every retained independent sector
readback is 512 bytes of `0x6B`, SHA-256
`789a49fcfe20dccddb0f9266345989ae51c3333847df3bc45400d1b04033a565`.
The final ignored run is `build/fwbug-008/mos-tests-final/`; `results.json`
hashes to
`2df7210ba5c3c32624a3ad6d9f43de2f8af813ffbc609d7a108fb0fee45c7bca`.
This is emulator evidence only.

## Physical evidence

Exact EMOS commit `8ecea5bc6cb4f9f563bc570316afbdaa08648632`
contains repair commit `19b8b6f9e9983190edb7b0beca95355e257b6851`
plus the physical fixture diagnostics and corrected direct-RST bindings. The
128,579-byte flashed artifact has SHA-256
`7c7ac79fcbdb4a9d67885aede552e808e0111bcc7e2d54b012c43add0305317a`.
The independent 131,072-byte installed-ROM readback exactly matched the padded
candidate and has SHA-256
`4fab4a413ff3e7d163ee8bd605ef9390503ba25db116a5c9e6137e300932a729`.

The first fixture attempt returned locked status 2 before any raw write. Review
of the linked AgonDev library showed that its `sd_writeblocks` symbol dispatches
through MOS's direct C function table, not public RST selector `0x73`; its
`sd_getunlockcode` binding also does not accept the explicit destination needed
by this oracle. The fixture therefore uses project-local assembly bindings for
documented RST selectors `0x70`, `0x71` and `0x73`. Independent observation and
restoration continue through the distinct C function-table controls. The final
fixture is 10,492 bytes and has SHA-256
`9ed1064702f339ca8b6f3bb4ba852cdb9ccf724b59446ff4859b69eb85a47ce3`.

The authorized physical run proved sector 2 lies before the first partition at
LBA 8192. RST `0x73` returned zero; independent readback matched the generated
pattern CRC32 `3b3befd6`. The C control restore and its independent readback
both returned zero and matched preimage CRC32 `b2aa7578`. The fixture recorded
`write_attempted=1`, `status=pass` and `detail=completed`. The paired Extender
runner restored the exact original startup and completed its full 55-case
retained closure. Durable bench evidence is intentionally retained under the
Extender repository's ignored hardware-validation directory.

A later full-suite reuse exposed a protocol defect outside the accepted raw
write repair: the host waited 120 seconds for a result service that the startup
file did not reliably launch after `RUN`, then treated the timeout as permission
to reset. The corrected fixture itself starts `EMOS sdserve --fast /` only after
it has either made no raw write or independently verified restoration. Missing
service is no longer considered a safe reset boundary. This fixture correction
requires fresh physical validation; it does not alter production EMOS keyboard
defaults or the accepted `src/mos_api.asm` repair. Exact fixture commit
`150480e8054259131a00ad1b412d389b36339889` builds a 10,650-byte binary with
SHA-256 `6e04a760f87ae503be57f4991c2a92c9128da32f7882fb6dee88f976d856ef81`.

The first revalidation run against EMOS `13951a29` again produced a complete
passing raw-media record: selector `0x73`, independent observation, independent
C restoration and restoration readback all returned zero; CRC32 values remained
`3b3befd6` for the pattern and `b2aa7578` for the restored preimage. The host did
not receive the positive listener handoff because the P4 retained ExCom routing
across the eZ80 reset. `VDU 22 3` selected geometry but did not return EMOS
transport ownership to Legacy, so `ext.sdlink` correctly returned
`EMOS_UNAVAILABLE` (35). The retained result was recovered only after an
explicit `EMOS LEGACY` command.

The fixture recovery startup and Extender-owned one-shot launcher now issue
`EMOS LEGACY` after selecting Extender keyboard input and before `VDU 22 3`.
This is a test-protocol correction: it does not change the accepted raw-write
repair or production EMOS boot defaults. The corrected fixture is 10,663 bytes
with SHA-256
`5b0945ffcc5db2c37beb61c5aa4f0c749c8d220cc8a9364a4d7bbd9e0998fb4d`.
Its build and source-order guard pass. The next run proved the raw operation
and restoration again passed, but exposed a second protocol error: an ordinary
running application cannot recursively launch `EMOS sdserve`; the EMOS utility
dispatcher correctly returns busy until that application has returned.

Exact fixture commit `888c9210619c50bdea507c3f6de134cf239f0969` removes the invalid nested
utility call. The host supplies a fresh 16-digit hexadecimal token to `RUN`;
after all raw I/O has ended, the fixture writes that token into its result file,
prints `A10-RP04 COMPLETE <token>`, and returns. The host must observe that exact
marker followed by a MOS prompt before it may start the result listener. The
fixture builds to 10,747 bytes with SHA-256
`4f83e462cad913537d0f0103468423f151d271531177a849fdfb8434a8ec64e6`;
its focused source contracts pass. Fresh physical validation remains.

## Completion

Close only after the Author accepts the candidate. The passing physical result
is hardware proof for this bounded repair, not production promotion.
