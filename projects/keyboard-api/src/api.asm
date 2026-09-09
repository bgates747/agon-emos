; Ordinary MOS API fixture. No private EMOS entry, GPIO or UART access.
; The callback is an assembly ABI; C runs behind an explicit DE/stack bridge.
    .assume adl=1
    .section .text
    .global _probe_callback
    .global _probe_getkey
    .extern _probe_record
    .extern _callback_irq_enabled

_probe_getkey:
    xor a
    rst.lil 08h
    ret

_probe_callback:
    ; Record interrupt state before any instruction could change it. Never EI.
    ld a,i
    ld a,0
    jp po,probe_irq_saved
    inc a
probe_irq_saved:
    ld (_callback_irq_enabled),a
    push de
    call _probe_record
    pop de
    ; Deliberately exercise the stock assembly callback's register freedom.
    ; EMOS must protect the interrupted C frame, including alternate registers.
    ld ix,0123456h
    ld iy,0654321h
    exx
    ld bc,0112233h
    ld de,0445566h
    ld hl,0778899h
    exx
    ex af,af'
    xor a
    ex af,af'
    ld bc,0123456h
    ld de,0654321h
    ld hl,0112233h
    ret
