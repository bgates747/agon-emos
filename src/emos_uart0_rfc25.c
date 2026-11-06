/* INTEG-021 private admission. Assembly captured actual IFF and MB before DI;
 * all publication and route checks occur in that same critical section.
 * Callback means source release only; foreground guards retire TEMT tail.
 * A live netp OPEN is not physical ownership and does not block UART0 graphics.
 */
#ifdef EMOS_UART0_RFC25
#include "emos.h"
#include "ff.h"
#include "emos_sdlink.h"
#include "emos_parallel.h"
#include "emos_parallel_handover.h"
extern BYTE emos_rfc25_application;
extern volatile BYTE emosVduBackend;
extern BYTE emos_uart0_async_owned(void);
extern BYTE emos_rfc25_owned;
BYTE emos_rfc25_admit(UINT24 source,UINT24 length,UINT24 callback){
    if(!emos_rfc25_application||!emos_application_context())return EMOS_UNSAFE_CALLER;
    if(!length||!emos_application_span(source,length)||!emos_application_span(callback,1))
        return FR_INVALID_PARAMETER;
    if(emos_get_mode()!=EMOS_MODE_LEGACY||emosVduBackend)return EMOS_UNAVAILABLE;
    if(emos_uart0_async_owned()||!emos_parallel_boot_uart_allowed()||
       emos_parallel_boot_handover()->phase!=EPH_UART)return EMOS_BUSY;
    emos_rfc25_owned=1;return FR_OK;
}
#endif
