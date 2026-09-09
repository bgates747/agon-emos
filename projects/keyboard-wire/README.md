# Controlled P4 keyboard exerciser

Owner: INTEG-009 paired P4 increment, coordinated with Extender PORT-005.
Identity: `uart-keyboard-probe-r01`, Author-approved with registry r38.
EMOS remains the reviewed v0.1.8 image.

`KBWIRE.BIN` is an ordinary SD-loaded C application using documented MOS calls.
It selects layout 1, installs a callback, selects browser input with
`EMOS KEYINPUT browser`, and waits for twelve unsolicited stock keyboard
packets. EMOS owns UART1 and keyboard publication; the application neither
accesses UART registers nor writes sysvars or the virtual-key map.

The controlled sender supplies a down/up, Shift down, B down three times,
B up, Shift up, 7 down/up and Enter down/up. P4 supplies each event to the
retained VDP acquisition/callback/event/serializer path, with 500 ms after
readiness and 250 ms between completed key transmissions. The application
checks payloads, callback-before-publication counts, interrupt state,
independently held keys, modifiers, repeated downs, final empty map and clock
progress. It restores mainboard input and removes its callback on every exit.
Missing keys have a ten-second clock deadline plus a stopped-clock loop bound.
No interactive getkey/editor wait can leave this bench fixture stuck.

Autoexec selects the display mode, runs the same-build EMOS smoke program,
then the separately identified application:

```text
VDU 22 3
LOAD /bin/EMBOOT.BIN
RUN
LOAD /bin/KBWIRE.BIN
RUN
EMOS KEYINPUT
```

The application writes one result byte to `/keyboard-state.bin` (1 pass,
255 fail). This is a test result through the normal file API, not persistent
keyboard configuration or a product acknowledgement. Missing input must
report FAIL and restore mainboard input; stock autoexec stops on that error.

`scripts/prepare_keyboard_api_review.py --fixture wire --packets PATH` builds
only this application against a hash-checked existing EMOS bundle. The packets
file comes from Extender's maintained sender host test, which exercises the
retained serializer. The controlled peer replays those bytes through Fab's
UART1 adapter; actual EMOS interrupts, parser, callbacks and map updates run on
the emulated eZ80. Separate CLI checks prove the twelve events and the SD
program's deadline/mainboard cleanup when readiness succeeds but keys never
arrive. These precede the generated graphical review
profile. Use its `./fab-agon-emulator` entry point.

This does not emulate the P4 binary or prove physical baud, CTS/RTS, electrical
margin, browser focus/network ownership or layout translation. P4 hardware
qualification remains separate. Stock API/editor and recovery cases already
covered by `keyboard-api` are not repeated here.
