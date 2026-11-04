# Isolated Eirene-based EMOS port

This task branch is an offline development candidate. It starts at Eirene RC2
825eaf7 and imports EMOS6ececd56 behavior. No installed consumer is changed.

Active composition matches the controller+whole-file parallel upload carrier;
unused/retired source is excluded explicitly. Native Eirene startup, sysvars,
FatFS and async sender remain the foundation. EMOS owns RST10/18 output and
51h/getfunction20h gateway. Async42h/43h remain explicit UART0-only calls;
ExCom submission returns BUSY and a live sender blocks EMOS route changes.
The application must finish before ordinary output, returning or replacing
UART0 handlers. No implicit app-exit cleanup is introduced in this first cut.

Initial leaf conversion is promoted to human-maintained native source here;
no generated prepared tree was edited. The temporary emos_z80_io.h adapter
reuses the reviewed limited I/O lvalues for imported EMOS units only. Native
Eirene core retains its own numeric I/O conventions. Remove the adapter after
those units are ported directly; never include both in one translation unit.

Compatibility and full byte validation remain unqualified until the task
checks pass and the Author separately authorizes deployment.
