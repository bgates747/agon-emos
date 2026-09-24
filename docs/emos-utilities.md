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
This increment has no physical acceptance or SD migration claim. The existing
sdserve MOSlet may be used as a launcher check, but moving an already disk-based
program does not recover ROM. Existing installed listener paths remain intact.
