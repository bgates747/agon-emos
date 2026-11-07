; Optional app-RAM observer bridge: no timer, recorder or serializer in ROM.
; Called under DI. A=event, B=drain remaining, C=existing raw sample where
; meaningful. Supply D=PC_DR, E=RX hold, HL=immediate six-byte parser prefix.
; Admitted marker preserves all registers and IFF; bridge preserves its own
; context construction. Valid app entries have high byte04..0a, NULL has0.
.assume adl=1
.include "equs.inc"
.text
.global _emos_neta_diag_emit
.extern _emos_neta_diag_hook,_emos_neta_rx_hold,_emos_key_rx
_emos_neta_diag_emit:
 push af
 ld a,(_emos_neta_diag_hook+2)
 or a
 jr z,diag_none
 pop af
 push de
 push hl
 push ix
 push af
 in0 d,(PC_DR)
 ld a,(_emos_neta_rx_hold)
 ld e,a
 ld hl,_emos_key_rx
 ld ix,(_emos_neta_diag_hook)
 pop af
 call diag_app
 pop ix
 pop hl
 pop de
 ret
diag_none:
 pop af
 ret
diag_app:
 jp (ix)
