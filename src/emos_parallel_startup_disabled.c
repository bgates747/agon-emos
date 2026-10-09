/* Ordinary UART-only source profile: intentionally inert startup boundary.
 * Candidate recovery lives in the separately selected owner source unit. */
#include "emos_parallel_handover.h"
void emos_parallel_boot_start(void) { }
void emos_parallel_boot_connect(void) { }
BYTE emos_parallel_boot_uart_allowed(void) { return 1; }
