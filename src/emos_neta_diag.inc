; INTEG-024 diagnostic-only app marker, source plan NET-004 RX-DIAG-R01.
; OFF emits no instructions or references. Marker preserves all interrupted
; primary/alternate/ADL registers; existing source owns IFF and UART samples.
.ifdef EMOS_UART1_RX_DIAGNOSTIC
.extern _emos_neta_diag_emit
.endif
.macro EMOS_DIAG_MARK event
.ifdef EMOS_UART1_RX_DIAGNOSTIC
 push af
 ld a,\event
 call _emos_neta_diag_emit
 pop af
.endif
.endm
