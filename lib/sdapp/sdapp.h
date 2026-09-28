/* Synchronous ADL C helper; link into the caller, never load another MOSlet.
 * Paths are absolute ASCII <=120 bytes. Receive destination <=112 for the
 * existing recoverable sibling names. Caller keeps interrupts enabled and
 * enters only at a complete VDU boundary. ExCom requires prior resident
 * capability negotiation. The paired P4 application-card worker is required. */
#ifndef EMOS_SDAPP_H
#define EMOS_SDAPP_H
#include <stdint.h>
enum { EMOS_FILE_RECEIVE=3, EMOS_FILE_SEND=4 };
enum { EMOS_FILE_OK, EMOS_FILE_BUSY, EMOS_FILE_UNAVAILABLE, EMOS_FILE_INVALID,
       EMOS_FILE_CANCELLED, EMOS_FILE_IO, EMOS_FILE_PROTOCOL,
       EMOS_FILE_UNCERTAIN, EMOS_FILE_RECOVERY };
typedef int (*emos_file_cancel)(void *user);
unsigned emos_file_transfer(unsigned direction,const char *source,const char *destination,
                            unsigned overwrite,emos_file_cancel cancel,void *user);
/* Platform adapter uses only EMOS gateway 0x51. Host tests substitute this
 * boundary, not the production transfer/file-engine implementation. */
unsigned sdapp_link(unsigned operation,const uint8_t *record,unsigned length,
                    uint8_t *output,unsigned capacity,unsigned *received);
uint32_t sdapp_clock(void);
#endif
