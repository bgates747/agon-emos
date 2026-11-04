# Eirene MOS — UART0 development

This fork adds an opt-in asynchronous VDP sender based on Tom Morton's small
candidate, with interrupt-driven16-byte OTIRX refills. An aware application can
calculate while MOS transmits its borrowed buffer. Ordinary startup and RST18
remain upstream Eirene paths; legacy applications retain synchronous output.

**Published candidate: Eirene MOS3.1.0 UART0 RC1**, based on upstream3.1.0_beta_1.
This is a candidate in bgates747's fork, not an upstream Eirene release.
Development and candidate tags stay on `codex/uart0-async`; `main` tracks upstream.
The exact branded RC1 image passed the final stock-VDP Console8 check.
The working tree develops a separate, unpublished **TRIM Trial1**, reducing
sender size and moving extra callback protection into the app. It has different
caller preconditions; see [TR-001](docs/tasks/TR-001/README.md) and the
[trial contract](experiments/uart0-trim/contract.md). It is not yet hardware qualified.
[Download the RC1 prerelease](https://github.com/bgates747/eirene-mos/releases/tag/3.1.0_uart0_rc_1);
[qualification and artifact pins](docs/tasks/RC-001/publication.md).

The pre-branding implementation passed the five-MOS Console8 comparison:
[physical results](docs/tasks/TM-001/hardware-perf2-r01.md). It kept85.5% of the
fixture's no-traffic foreground calculation rate while sending flat out, at
essentially stock payload throughput. This is useful calculation overlap, not
an exclusive CPU-utilization measurement or a bulk-speed improvement claim.
Three short repeats per scenario do not establish indefinite sustained rates.

[Caller contract](docs/uart0-async.md), [original C example](examples/uart0-async/)
and [no-argument regression fixture](experiments/uart0-release/).
The async syscall is fork-local; no upstream ABI allocation is implied.
Only Console8 and the documented Flexe configurations are covered by our
qualification. EMOS, other boards and real application audio deadlines remain
separate work. The transport uses ordinary VDP commands; its sender adds no
VDP protocol extension. The exact RC passed two focused regression runs on stock Bistromathics VDP
2.16.0. Earlier performance benchmarks used the matched receiver-fixed VDP;
their timings are not stock-VDP measurements. See the
[final hardware record](docs/tasks/RC-001/hardware.md). The separate receiver
fix remains independent of this sender.

Part of the official Agon Platform organisation firmware for all Agon computers.

This firmware is intended for use on any Agon Light compatible computer.  As well as the Agon Console8, it has been tested on the Olimex Agon Light 2.

The Agon Platform firmware is a fork from the original [official Quark firmware](https://github.com/breakintoprogram/agon-mos) for the Agon Light.  It contains many extensions and bug fixes.  Software written to run on Quark firmware should be fully compatible with Agon Platform releases.

### What is the Agon

Agon is a modern, fully open-source, 8-bit microcomputer and microcontroller in one small, low-cost board. As a computer, it is a standalone device that requires no host PC: it puts out its own video (VGA), audio (2 identical mono channels), accepts a PS/2 keyboard and has its own mass-storage in the form of a micro-SD card.

https://www.thebyteattic.com/p/agon.html

There are several variants of the Agon, including the original Agon Light, the Olimex Agon Light 2, the Agon Light Origins Editions, and the [Agon Console8](https://www.heber.co.uk/agon-console8).

The Agon Console8 is a fully compatible variant of the Agon Light from the same designer, with the addition of a built-in PS/2 mouse port and two Atari-style DB-9 joystick ports.

### What is a MOS

The MOS is a command line Machine Operating System, similar to CP/M or DOS, that provides a human interface to the Agon file system.  It runs on the eZ80 CPU of the Agon.

It also provides an API for file I/O and other common operations for BBC Basic for Z80 and other third-party applications.

### Loading BBC Basic for Z80

1. Download bbcbasic.bin from [agon-bbc-basic releases](https://github.com/breakintoprogram/agon-bbc-basic/releases)
2. Copy it to the `/bin` directory of your Agon's SD card
3. Insert the SD card into the AGON and reset/boot it
4. Run BBC BASIC by typing `bbcbasic` at the MOS command prompt
5. You should then be greeted with the BBC Basic for Z80 prompt

### Etiquette

Reporting issues and pull requests are welcome.

A Contributing guide will be added in due course.

### Build

Eirene uses **AgonDev**, an open-source LLVM/Clang eZ80 toolchain. The inherited
ZDS II instructions described the older official MOS and do not build this
Eirene tree. Install the AgonDev toolchain and its matching Agon libraries;
then build from the repository root:

```sh
make AGONDEV_TOOLCHAIN=/absolute/path/to/agondev
```

If `agondev-config` is installed, plain `make` uses its configured prefix.
Outputs are `bin/mos.bin` and `bin/mos.map`. No HEX conversion is required.
The RC package records the exact source commit, compiler/library identities,
commands and artifact checksums. The developer release-preparation recipe is
[here](experiments/uart0-release/README.md).

For the original ZDS II upstream MOS, continue using its original compiler;
this Eirene build does not migrate or replace that separate project.

### Testing the MOS

#### Using the Agon Emulator

MOS can be tested out without the need to reprogram your Agon Console8 or Agon Light by using the [Fab Agon Emulator](https://github.com/tomm/fab-agon-emulator).

To test using the emulator, create a new `MOS.bin` file and place that into the emulator directory and run the emulator.  The emulator will automatically load the `MOS.bin` file and run it.

It should be noted that the emulator is not 100% accurate, so some features may not work as expected, but it is a very close simulation.

Unless you are using the ZDS II tools to program the eZ80 directly, it is recommended that you test your MOS on the emulator before testing on real hardware.

#### Flashing your Agon Light or Agon Console8

The MOS can also be flashed on your device using the [agon-flash Agon MOS firmware upgrade utility](https://github.com/AgonPlatform/agon-flash).  This is a command line utility that runs on the Agon itself, and can flash MOS to the eZ80 from a file stored on your SD card.

In case of emergency, such as flashing an experimental build of MOS you have built that does not work, there is an [Agon recovery utility](https://github.com/AgonPlatform/agon-recovery) that can be used to revert the Agon back to a known working version of the MOS.

It is recommended when using the agon-flash utility that you use a filename other than `MOS.bin` for your new experimental MOS version, and keep a known working version of the `MOS.bin` file in the root of your SD card.

#### Programming the eZ80 directly

To program the eZ80 directly using ZDS II you will need a Zilog Smart Cable to connect to the ZDI connector on the board.  These are available from online stockists such as Mouser or RS Components.  Please note however that development of the MOS for the Console8 has *not* been conducted using a Zilog USB Smart Cable.

There are three compatible cables with the following part numbers:

- `ZUSBSC00100ZACG`: USB Smart Cable (discontinued)
- `ZUSBASC0200ZACG`: USB Smart Cable (in stock - this requires ZDS II version 5.3.5)
- `ZENETSC0100ZACG`: Ethernet Smart Cable (in stock)

Important! Make sure you get the exact model of cable; there are variants for the Zilog Encore CPU that have similar part numbers, but are not compatible with the Acclaim! eZ80 CPU.

Any custom settings for Agon development is contained within the project files, so no further configuration will need to be done.

Other options for programming the Agon are available, and the community will be happy to advise on these.

### Documentation

The Agon Platform documentation can be found on the [Community Documentation](https://agonplatform.github.io/agon-docs/) site.  This site provides extensive documentation about the Agon Platform firmware, covering both Quark and Console8 firmware releases.

### Community

There is a [vibrant and active community on Discord](https://discord.gg/7Ruseg98T9), where you can get help and advice on developing for the Agon.

There is also the [Agon Programmers Group on Facebook](https://www.facebook.com/groups/667325088311886).

### Licenses

This code is released under an MIT license, with the following exceptions:

* FatFS: The license for the [FAT filing system by ChaN](http://elm-chan.org/fsw/ff/00index_e.html) can be found here [src_fatfs/LICENSE](src_fatfs/LICENSE) along with the accompanying code.

### Additional Links

- [Zilog eZ80 User Manual](http://www.zilog.com/docs/um0077.pdf)
- [ZiLOG Developer Studio II User Manual](http://www.zilog.com/docs/devtools/um0144.pdf)
- [FatFS - Generic File System](http://elm-chan.org/fsw/ff/00index_e.html)
- [AVRC Tutorials - Initialising an SD Card](http://www.rjhcoding.com/avrc-sd-interface-1.php)
