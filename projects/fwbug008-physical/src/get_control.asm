; Bind the raw-SD calls deliberately. AgonDev's sd_readblocks/sd_writeblocks
; library functions dispatch through MOS's C function table, so they cannot
; exercise the repaired RST 08h selector 73h. Its sd_getunlockcode wrapper also
; lacks an explicit destination argument. These fixture-local bindings follow
; the documented RST API layouts; the control lookup remains independent.
 .assume adl=1
 .section .text
 .global _fwbug008_get_unlock
 .global _fwbug008_init
 .global _fwbug008_rst_write
 .global _fwbug008_control_write

; void fwbug008_get_unlock(uint24_t *destination)
_fwbug008_get_unlock:
 push ix
 ld ix,0
 add ix,sp
 ld hl,(ix+6)
 pop ix
 ld a,070h
 rst.lil 08h
 ret

; uint8_t fwbug008_init(const uint24_t *unlock)
_fwbug008_init:
 push ix
 ld ix,0
 add ix,sp
 ld hl,(ix+6)
 pop ix
 ld a,071h
 rst.lil 08h
 ret

; uint8_t fwbug008_rst_write(const void *sector_and_unlock,
;                            uint8_t *buffer, uint16_t count)
_fwbug008_rst_write:
 push ix
 ld ix,0
 add ix,sp
 ld hl,(ix+6)
 ld de,(ix+9)
 ld bc,(ix+12)
 pop ix
 ld a,073h
 rst.lil 08h
 ret

; Return MOS's independent C SD_writeBlocks entry (function-table slot 2).
_fwbug008_control_write:
 ld a,050h
 ld bc,0200h
 rst.lis 08h
 ret
