# Foreground EMOS utilities

EMOS v0.1.19 adds an explicitly prefixed way to launch ordinary MOSlets:

```
emos utility arguments
```

EMOS first checks its resident commands, then loads `/emos/utility.bin` at the
stock MOSlet address `0xB0000` and executes it through stock `mos_runBin`.
The command is case-insensitive. Utility names contain 1–24 ASCII letters,
digits, underscores or hyphens; EMOS lowercases the filename. Supply the name
without `.bin`. Paths, dots, wildcards and variable syntax are rejected.
`Moslet$Path` and `Run$Path` are unchanged: installing a utility in `/emos` does
not make its bare name a new stock MOS command.

Only an idle CLI or its autoexec may launch a disk utility. An application or
MOSlet calling `mos_oscli("emos utility ...")` gets `EMOS_BUSY` (31), preventing
nested loading over the live MOSlet or its stack. Existing resident commands
retain their own admission rules; their behavior is not moved onto SD.

A utility is an ordinary trusted executable linked for `0xB0000`, with its code,
data, heap and stack within `0xB0000..0xB7FFF`. The launcher accepts files from
69 through 32768 bytes, rejects directories and oversized files, invalidates
old slot header bytes before loading and uses stock executable-header checking.
The size ceiling is not proof of sufficient runtime heap/stack. A stock MOS
header cannot prove the link address, integrity or required EMOS version.
Utilities must clean up and return; no resident callback into their code is
supported. There is no relocation, provider registry, swap file or background
execution mechanism.

The remaining argument string and return value follow stock load/run behavior.
AgonDev's ordinary startup code performs its own argv parsing, including its
existing splitting of quoted whitespace. EMOS does not invent another parser.
Missing files/cards and load errors propagate stock MOS/FatFS codes. The
launcher retains stock file-read semantics; it does not add transactional
loading or an executable integrity/authentication scheme.

Resident keyboard, display-mode and service commands remain usable independently
of utility files. `DISCOVER` and `CLEAR` are reserved retired commands returning
`EMOS_UNAVAILABLE` (35), not utility names. External `.emo` providers are no
longer loaded; unknown service identities return `EMOS_NOT_FOUND` (27), while
API `0x51`, C function slot `0x20` and resident services remain in firmware.

Validation and deployment status are tracked in [AUDIT-008](tasks/AUDIT-008.md).
On 2026-09-24 the Author authorized physical deployment: complete ROM readback,
ExCom/Legacy, mixed-case utility dispatch, listener transfers/reentry and a 4096-byte
application sentinel passed. The maintained listener is now `/emos/sdserve.bin`,
invoked with `EMOS sdserve [--fast] /`. Moving it itself saves no ROM. Broader
human/native-keyboard acceptance remains distinct from these bounded checks.

## Finite external-job handoff (development only)

Resident `ext.sdlink` operations 5/6 provide the claimed-utility binding and final
control-sequence handoff. Ordinary application/manual callers cannot acquire it.
The utility must explicitly report completion; returning zero alone is failure.
The finite [sdjob utility](../projects/sdjob/README.md) and P4 runtime connection
are implemented and host-tested; hardware qualification remains open.
This does not replace or change the production `EMOS sdserve` procedure.
The owning contract and current implementation boundary are recorded in
[Extender admission](../../agon-extender/docs/tasks/REMOTE-005/ADMISSION-CONTRACT.md)
and [runtime results](../../agon-extender/docs/tasks/REMOTE-005/A08-RUNTIME-RESULTS.md).

## UARTFLOW diagnostic extraction (development only)

`EMOS UARTFLOW` now uses `/emos/uartflow.bin` in the PORT-008 development
candidate. Sequence, deadlines and reporting moved to the MOSlet; ownership and
exit cleanup remain resident. UARTTEST and VDPPOLL now also use `/emos/uarttest.bin` and `/emos/vdppoll.bin`
in the newer off-bench composition. Their [guide](../projects/uartprobe/README.md)
records the paired service dependency and unperformed hardware validation.
VDPTEXT and the text-probe service remain resident.
The [UARTFLOW guide](../projects/uartflow/README.md) records build, peer and input
requirements. The 2026-10-08 Author-authorized deployment passed full ROM/utility verification,
real paired success and wrong-peer cleanup/input reacquisition. This remains a
development candidate awaiting Author review, outside selected production.
