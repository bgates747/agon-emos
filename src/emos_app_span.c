#include "emos_sdlink.h"

#ifdef EMOS_NETWORK_PARALLEL
/* INTEG-020/021 shared ordinary-application bounds. Controller, netp and RFC25
 * use the same RAM contract; do not inline duplicate 24-bit checks per service.
 * Unsigned offset rejects addresses outside that span before count subtraction. */
BYTE emos_application_span(UINT24 p,UINT24 n){
    UINT24 offset=p-0x040000;
    return offset<0x070000&&n<=0x070000-offset;
}
#endif
