/* INTEG-020 physical-owner boundary, NOT a raw app GPIO API.
 * No default/UART implementation: enabling EMOS_NETWORK_PARALLEL requires a
 * matched owner binding. Source/target lifetimes are synchronous through return.
 * Link owner alone parks/restores shared pins and keeps unrelated IRQs serviced.
 * All NM bytes use parallel. UART control contains only pipe metadata.
 */
#ifndef EMOS_NETP_TRANSPORT_H
#define EMOS_NETP_TRANSPORT_H
#include "emos_netp.h"
/* OPEN returns a nonzero nonreused P4-bound token after actual admission. */
extern UINT32 emos_netp_token; /* One resident ownership identity, no second mirror. */
BYTE emos_netp_link_open(void);
BYTE emos_netp_link_submit(const BYTE *record,UINT16 length);
/* SMALL sets required length without touching destination/consuming reply.
 * Success publishes a whole record only after physical completion/pad release.
 * Uncertain transfer returns FAULT, never replays. BUSY admits no transfer.
 */
BYTE emos_netp_link_take(BYTE *out,UINT16 capacity,UINT16 *length);
/* Success proves socket owner and pipe quiescent. BUSY/failure retains token. */
BYTE emos_netp_link_release(void);
#endif
