; INTEG-014 diagnostic app bindings with temporary interrupt observation.
; AgonDev C ABI: 3-byte stack slots; IX/IY preserved. Read/write operations
; use the pinned original EMOS routines; no direct UART/GPIO register access.
; Counted A status is specific to the pinned EMOS v0.1.12 implementation;
; carry differs by route. Delimiter output cannot expose intermediate failure.
        .assume adl=1
        .section .text
        .global _bench_clock
        .global _bench_count

_bench_clock:
        ld hl,3
        add hl,sp
        ld hl,(hl)
        ld hl,(hl)              ; one atomic low-24-bit sysvar read
        ret

_bench_count:
        push ix
        ld ix,0
        add ix,sp
        push iy
        ld hl,(ix+6)
        ld bc,(ix+9)
        rst.lil 18h
        ld hl,0
        ld l,a                  ; both pinned routes return A=0 on success
        pop iy
        pop ix
        ret

; Private ISR callback: preserve registers and copy only through the bounded C
; receiver. No VDU, MOS, formatting or SD operations from this callback.
        .global _graphics_callback
_graphics_callback:
        push af
        push bc
        push de
        push hl
        push ix
        push iy
        push de
        call _graphics_receive
        pop de
        pop iy
        pop ix
        pop hl
        pop de
        pop bc
        pop af
        ret

; INTEG-014 test-only observers. Both original vector owners are checked before
; install/restore. No source/route transition occurs while these are installed.
        .include "build/firmware_abi.inc"
        .global _diag_rom_first
_diag_rom_first:
        ld a,(0)
        ret
        .global _diag_lock
        .global _diag_unlock
        .global _diag0_irq
        .global _diag1_irq
        .global _diag_uart0_put
_diag_lock:
        ld a,i
        di
        jp po,1f
        ld a,1
        ret
1:      xor a
        ret
_diag_unlock:
        ld hl,3
        add hl,sp
        ld a,(hl)
        or a
        ret z
        ei
        ret
_diag_uart0_put:
        jp ROM_MAINBOARD_PUT

_diag1_irq:
        di
        push hl
        ld hl,(_diag1_count)
        inc hl
        ld (_diag1_count),hl
        pop hl
        jp ROM_UART1_IRQ

; Keep the maintained full register-save envelope and stock read/parser calls.
; Additional work is only a counter and bounded raw-byte observation in app RAM.
_diag0_irq:
        di
        push af
        push bc
        push de
        push hl
        push ix
        push iy
        ex af,af'
        push af
        exx
        push bc
        push de
        push hl
        ld hl,(_diag0_count)
        inc hl
        ld (_diag0_count),hl
        call ROM_UART0_RX
        ld c,a
        ld hl,(_diag0_left)
        ld de,0
        or a
        sbc hl,de
        jr z,2f
        dec hl
        ld (_diag0_left),hl
        ld hl,(_diag0_cursor)
        ld (hl),c
        inc hl
        ld (_diag0_cursor),hl
        jr 3f
2:      ld a,1
        ld (_diag0_overflow),a
3:      ld hl,ROM_PROTOCOL_DATA
        call ROM_PROTOCOL
        pop hl
        pop de
        pop bc
        exx
        pop af
        ex af,af'
        pop iy
        pop ix
        pop hl
        pop de
        pop bc
        pop af
        ei
        reti.l
