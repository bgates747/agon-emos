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

## Network admission development successor

INTEG-019 adds the ext.net fixed provider in experimental r03 on corrected
native r02 source16ec1f78. The application owns codec/poll/game decisions; P4
owns sockets. Existing async UART, keyboard, controller, SD and parallel units
remain. Root firmware-check passes all five mandatory link gates; ROM adds1964B
and static RAM148B. Identified compilation and physical matched qualification
are separate; prior installed r02 remains the rollback until a deliberate flash.
