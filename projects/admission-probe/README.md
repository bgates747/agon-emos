# Admission dispatch probe

Test-only MOSlet for REMOTE-005 A04. Never install as a real file service.
Build with `make AGONDEV_TOOLCHAIN=<toolchain>`. In an isolated emulator SD,
copy bin/admissionprobe.bin to `/emos/sdjob.bin`. Seed `/extender/sentinel.bin`
with 4096 bytes `(index * 17 + 3) & 255`, then autoexec must issue
`VDU 22 3`, `LOAD /extender/sentinel.bin 0x40000` and `EMOS KEYINPUT extender`.
The controlled UART peer supplies keyboard readiness and admission replies.
Run scripts/admission_peer.py with --case utility and its explicit runtime,
firmware, SD and result paths. It supplies `proof` to the public application
editor and checks return to CLI. Other peer cases use a separate SD without
sdjob.bin. Mode selection belongs in autoexec, never this program.

Success proves bounded loader/editor/memory behavior, not filesystem transfers,
P4 compatibility or physical timing. Human emulator review is required before
committing emulator-coupled changes.
