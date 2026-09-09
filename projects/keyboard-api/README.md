# Resident keyboard API exerciser

Identity: `keyboard-api-probe-r01`, Author-approved with registry r36.
Owner: [INTEG-009 Work 3](../../docs/tasks/INTEG-009.md).

`KBAPI.BIN` is an ordinary SD-loaded MOS program. It selects browser input
through `EMOS KEYINPUT`, installs a documented keyboard callback, reads the
normal sysvars/map and calls stock getkey/editor APIs. It never owns UART1,
calls private firmware symbols, writes sysvars or changes the video mode.
The existing v0.1.8 firmware is reused byte-for-byte. Its build identity and the
separate fixture identity appear in the review manifest and boot output.

The controlled host peer sends stock packets through an isolated Fab UART1
adapter. EMOS's actual UART1 interrupt, resident parser and stock assembly
effects execute on the emulated eZ80. The separate `keyboard-state.bin` file
is a one-byte test-stage signal written through normal MOS file calls. It is
not a product per-key ACK, stored preference or alternate keyboard protocol.

The fixture checks callback-before-publication, the DE payload and callback
edits, primary/alternate/index register preservation, modifiers, independent
held keys in the map, 260 events across counter wrap, five-byte settings with
no new completion flag, getkey ignoring releases, line editing and Escape,
callback removal, locale retention across source changes, clock progress and
return to mainboard input. Stock getkey/editor waits are unbounded; the host
enforces a 45-second overall failure deadline. All callback exits are cleaned
up before the application returns normally or reports an assertion failure.

`scripts/prepare_keyboard_api_review.py` builds only the fixture, checks the
selected firmware/runtime hashes, runs the paired CLI test and creates a frozen
graphical profile. Launch that profile through its generated
`./fab-agon-emulator` wrapper. It starts the same controlled peer, leaving the
window open after PASS. A new launcher invocation repeats the test with a fresh
peer and stage file. `last-peer.json` and `.log` record graphical execution.

The selected Fab 1.2.3 runtime omits UART1 RX interrupts. The generic
`mos-agondev/scripts/prepare_uart_peer.py` adaptation adds that interrupt and
an explicit Unix-socket peer in an isolated source copy. It also records every
changed-source and executable hash. The official runtime and independently
maintained Fab fork remain untouched. CTS is constant ready; FIFO overflow,
RTS backpressure, physical baud, browser mappings and P4 firmware are **not**
qualified here. See mos-agondev TEST-001 for the adaptation's removal condition.

Fab CLI's fake VDP stops processing display requests while it types a whole
stdin line. The runner lets the editor's post-prompt mode query finish before
sending its final CLI verification command. This is test pacing, not a change
to stock MOS keyboard buffering or production behavior.
