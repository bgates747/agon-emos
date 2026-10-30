# External UART and text diagnostic utilities

Development PORT-008 composition only; not installed or production-qualified.
All three commands load from SD. Build with the same AgonDev toolchain used
for UARTFLOW:

```
make NAME=uarttest
make NAME=vdppoll
make NAME=vdptext
```

Install `bin/uarttest.bin` as `/emos/uarttest.bin` and `bin/vdppoll.bin` as
`/emos/vdppoll.bin`, and `bin/vdptext.bin` as `/emos/vdptext.bin` only during an
authorized deployment of the paired EMOS.
Invoke `EMOS UARTTEST`, `EMOS VDPPOLL` or `EMOS VDPTEXT` from the idle Legacy
CLI. Each accepts
`--version`; other arguments return 19. Diagnostic success returns 0; failure,
including unavailable service, wrong mode or busy UART, returns 15. The command
no longer prints the resident build identity before running. Missing files
propagate ordinary MOS/FatFS load errors. All occupy B0000..B7FFF and use the
existing EMOS utility loader without overwriting the application slot.

Before any test, the operator must select mainboard keyboard input through
an already working input path. An active Extender/browser input owner prevents
UART acquisition; no utility silently takes it over. These are foreground
finite diagnostics, not ways to test an active ordinary console session.

| Command | Dedicated peer and exact transaction | UART1 configuration |
|---|---|---|
| UARTTEST | P4 round-trip peer: `EMOS UART1 -> P4\r\n` / `ACK\r\n` | 115200 baud, 8N1, no flow control |
| VDPPOLL | EDP General Poll peer: `17 00 80 A5` / `80 01 A5` (hex) | 1152000 baud, 8N1, CTS/RTS |
| VDPTEXT | EDP visible text, flush `17 00 CA`, poll `17 00 80 A6` / `80 01 A6` | 1152000 baud, 8N1, CTS/RTS |

The original algorithms, expected bytes, MOS clock deadlines (120 TX, 600 RX,
24 quiet units) and stopped-clock fuse (65535 polls) are retained. The clock
advances by two units per VBlank: at nominal 60 Hz these are approximately
1 s, 5 s and 0.2 s. They are not millisecond counts. Per-call gateway overhead
changes polling cadence; host and instruction tests cannot qualify real peer
timing or throughput. Paired hardware validation remains required.

All three commands reuse UARTFLOW's API 0x51 gateway adapter and resident
`ext.uartdiag` v1 service. UARTTEST needs the additive fixed slow-open operation
5; older EMOS rejects it. EMOS owns UART configuration, pin admission and exit
cleanup. The MOSlets contain no UART/GPIO access or IRQ callback. Their shared
source is in `src/probe.c` and `src/visible_text.c`; both share the original
clock helper. Separate variant object directories prevent
building one command with the other command's entry point.

INTEG-017's text/fake extraction is an offline candidate. The installed v0.1.27
still implements VDPTEXT in ROM. The new utility has no special mode switch:
it requires Legacy and a separately prepared peer. It does not change screen
mode, display route, input selection or startup. This one-shot diagnostic's
fixed text deliberately contains the retained clear-screen VDU sequence.
No current bench invocation is authorized by these build instructions.

`edu.text-probe` is retired in the new EMOS source. Its historical browser-typing
and visible-text applications are not compatible with this candidate. Ordinary
application text uses MOS VDU output under EMOS routing. Fake-adapter mode tests
remain external host models; `EMOS FAKE` cannot select a simulated route.
