# Finite admitted SD job (development)

`sdjob` is a foreground EMOSlet used only by resident idle-CLI admission. It
reuses the checked sdserve file engine and never opens an independent UART.
This is not the production manual listener and has not been deployed/qualified
on hardware. Ordinary operation remains `EMOS sdserve /`.

Build from this repository with `make -C projects/sdjob`. The Makefile fixes
B0000..B7FFF. The resulting `bin/sdjob.bin` is intended for `/emos/sdjob.bin` only
after paired build identification and qualification. Resident EMOS supplies the
private `--admitted` invocation and live binding; typing the command manually
cannot obtain admission. Source `engine.c` and `gateway.c` include maintained
implementations rather than copying their filesystem/transport logic.

The utility validates descriptor fragments, declared operation and path scope,
then reports READY. Only matching file-session requests reach the checked engine.
It keeps ordinary integrity checks, refuses undeclared mutations and protects
its running utility. STATUS polls run in foreground, including engine progress
calls. Cancellation waits for the current file operation's recoverable boundary.
Unfinished stages remain for explicit recovery, never become silent success.

FINISH acknowledgement and the final control sequence are handed back to resident
EMOS, which sends CLOSE and restores the CLI. A lost acknowledgement means failure,
not a new automatically retried job. The initial implementation is Legacy-only;
no ExCom capability or application-origin service is advertised.

The cross-component host tests and qualification boundary live in
[Extender runtime results](../../../agon-extender/docs/tasks/REMOTE-005/A08-INTEGRATION-RESULTS.md).
The test compiles this actual utility and file engine with a host FatFS adapter,
then runs them against the actual P4 peer/Channel. Physical timing, FAT durability
and available stack must still be qualified on the bench.
