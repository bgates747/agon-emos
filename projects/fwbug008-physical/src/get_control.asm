; Return MOS's independent C SD_writeBlocks entry (function-table slot 2).
; The repaired path under test is RST 08h selector 73h; this lookup deliberately
; obtains the separate C control used to restore the preimage.
 .assume adl=1
 .section .text
 .global _fwbug008_control_write
_fwbug008_control_write:
 ld a,050h
 ld bc,0200h
 rst.lis 08h
 ret

