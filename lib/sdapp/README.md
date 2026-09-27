# Linked application file-transfer helper — development candidate

`emos_file_transfer()` runs synchronously inside an ADL C caller. It reuses
EMOS gateway 0x51 (`ext.sdlink`) and the checked foreground SD engine; it does
not load an EMOSlet, access UART registers or change display mode. Current normal
P4 firmware does not implement the required application staging service.

```c
#include "sdapp.h"
unsigned result = emos_file_transfer(EMOS_FILE_RECEIVE,
    "/p4/source.bin", "/mystuff/received.bin", 0, NULL, NULL);
```

RECEIVE means P4 source to Agon destination. SEND means Agon source to P4
destination. `overwrite` is 0 or 1. An optional cancellation predicate runs in
foreground; it must return promptly and may not switch mode, load applications,
or take over transport. No callback survives return. A reentrant call returns
BUSY without disturbing the original owner. The helper imposes no Escape binding.

Link `sdapp.c`, `gateway.c`, the existing `projects/sdserve/src/service.c`, and
`projects/sdserve/src/emos_gateway.asm`; include the sdapp and sdserve source
headers. `projects/app-transfer/Makefile` is the maintained example. File-engine
code is linked into the caller rather than added to resident flash. The helper
uses static working buffers, so it is deliberately single-owner/non-reentrant.

## Contract and limits

1. Caller executes with interrupts enabled, at a complete VDU command boundary.
   Legacy only initially; unsupported mode fails without changing it. The resident
   gate classifies actual application/MOSlet context; Core cannot claim the lease.
2. Paths are copied before transmission. Absolute ASCII file paths are limited to
   120 source bytes and 112 destination bytes (existing recovery siblings).
   No directories, wildcards, traversal, current-directory assumptions or fast mode.
3. The helper computes source size/CRC in one bounded-buffer pass, then transfers
   a second pass. Destination FINISH/ACTIVATE retain the existing checked semantics.
   Changing source bytes cannot silently pass destination CRC verification.
4. OK follows checked activation and matching terminal acknowledgement. Lost
   activation acknowledgement never returns OK. RECOVERY means staging or a commit
   may have occurred: inspect the retained transaction before another attempt.
   A failure before mutation can return INVALID, BUSY, UNAVAILABLE, IO or PROTOCOL.
   CANCELLED means cancellation occurred before staging; interrupted staging
   conservatively returns RECOVERY. UNCERTAIN identifies unconfirmed completion.
5. Cancellation waits through the current record; it does not abort an in-progress
   activation. The helper preserves recovery files on interruption and does not
   automatically retry uncertain operations or delete the only confirmed copy.
6. Matching control identity, CRC, opcode/sequence and a derived per-job file session
   are required. Unsolicited external requests cannot turn this caller into a server.
   P4 staging must separately enforce the descriptor's paths and overwrite flag.

## Resident extension

Existing operations 0–3 are unchanged. Operation 4 (`APP_OPEN`) has one input byte,
four output bytes and returns a nonzero monotonically increasing boot-local token.
It requires application execution, enabled interrupts, Legacy transport and no
active listener. Nested open is BUSY. The application owns the control/file mailbox
until CLOSE (operation 3) or application transition/reset. Counter exhaustion
requires restart; it never wraps into a reused token. No application pointer is
retained by EMOS. Existing EMOS gateway range validation remains in force.

The helper uses the token as control session/generation and both grant words;
P4 HELLO supplies a new link incarnation. Application controls carry origin 2.
APP_BEGIN fragments are contiguous and carry total/offset/CRC, with exact descriptor
bytes. Reply body is the common 28-byte prefix; P4 assigns a nonzero job on the
first accepted fragment and retains it thereafter. READY follows the last fragment.
The file session is CRC32 of that final prefix, using 1 if CRC is zero. Control and
file sequences are separate. FINISH appends the outcome byte; CLOSE retires the job.
All P4 replies must match the whole common prefix except HELLO fields and the
initial assigned job. These details refine Extender's A03 application contract.

## Evidence boundary

Host tests execute real helper and checked engine code against a controlled peer.
The eZ80 UART-peer fixture validates send, caller-memory sentinel and return to MOS.
The retained directory-backed emulator does not implement filesystem sync, so its
receive path is not a durability qualification. P4 spool integration, ExCom framing,
and production integration remain in the owning Extender task. Physical Legacy
qualification now passes both directions with Extender's RAM-only r59 diagnostic
peer, actual mainboard FAT, 4096-byte caller sentinel and external-request rejection. Do not advertise
this candidate as an available network file-transfer feature.
