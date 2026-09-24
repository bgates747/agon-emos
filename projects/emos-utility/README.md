# Foreground utility launcher regression probe

`emos-utility-probe-r01` is a local-only AUDIT-008 fixture. Build with AgonDev:

```
make AGONDEV_TOOLCHAIN=<toolchain>
```

Build a second copy in an isolated directory with `RAM_START=0x40000` and
`RAM_SIZE=0x70000`; do not reuse MOSlet objects with changed link settings.
The maintained source is identical; the second copy tests application-call
rejection. No mode change or transport access occurs inside either program.

`verify.py --cli <official-agon-cli-emulator> --firmware <candidate-MOS.bin>
--utility <MOSlet-probe.bin> --application <application-probe.bin>
--listener <existing-sdserve-MOSlet.bin> --work <fresh-local-directory>
--output <evidence-directory>` creates isolated directory-backed media and
records a complete transcript, input hashes and elapsed host seconds.

The harness uses the official Fab CLI runtime, as the maintained MOS port boot
checks do. Its fake VDP cannot qualify display behavior, UART timing or physical
keyboard operation. The listener check proves launch/admission and clean return
when Extender keyboard/transport is absent, not file transfer. Host filesystem
loading does not qualify physical FAT/SD errors. Separate host tests inject
filesystem error returns and exercise actual launcher/gateway code.

The probe checks a 4096-byte application-memory sentinel, arguments, static
initialization on repeat launches, resident built-in availability, rejection of
nested launch from both application and MOSlet, name/size/header boundaries,
retired provider errors, built-in precedence, and fake mode round-trip. This is
bounded evidence, not proof of arbitrary utility stack/heap safety.
