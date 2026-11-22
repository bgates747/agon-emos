; INTEG-019: imported NMI-001 API R01 (e000cf0b53017ce49496bd721e9c3d5e1e17154702bdb1c7b5cca94f4f1aba3d).
; NMI-001 R01: one cooperative ADL app, one DI Z80 guest, one F92 watchdog.
.assume adl=1
.text
.global nmi_api, _nmi_dispatch, _nmi_reset, _nmi_reset_early, _nmi_app_leave
.global _nmi_phase, _nmi_app_context
.extern __default_nmi_handler

WDT_CTL .equ 0x93
WDT_RELOAD .equ 0x94
CLAIMED .equ 1
GUEST .equ 2
CALLBACK .equ 3
PARKED .equ 4
RECOVERY .equ 5
FRAME_LEN .equ 33

nmi_api:
    ld a,i
    jp po,wrong_context
    ld a,mb
    or a
    jr nz,wrong_context
    ld a,(_nmi_app_context)
    dec a
    jr nz,wrong_context
    ; After SWITCH_A, one MOS CALL return precedes the mixed RST frame.
    push hl
    ld hl,6
    add hl,sp
    ld a,(hl)
    pop hl
    cp 3
    jr nz,wrong_context

    ld a,(_nmi_phase)
    cp CALLBACK
    jr z,busy
    ld a,b
    or a
    jr z,query
    dec a
    jr z,acquire
    dec a
    jr z,run_guest
    dec a
    jr z,resume_guest
    dec a
    jr z,release
    ld a,23
    ret

wrong_context:
    ld a,32
    ret
busy:
    ld a,31
    ret
invalid:
    ld a,19
    ret
wrong_state:
    ld a,33
    ret

query:
    ld de,0x314d4e
    ld hl,0x070101
    xor a
    ret

acquire:
    ld a,(_nmi_phase)
    or a
    jr nz,busy
    in0 a,(WDT_CTL)
    bit 7,a
    jr nz,busy
    call upper_hl
    cp 4
    jr nz,invalid
    ld (callback_address),hl
    ld (callback_context),de
    ld a,CLAIMED
    ld (_nmi_phase),a
    xor a
    ret

upper_hl:
    push hl
    ld hl,2
    add hl,sp
    ld a,(hl)
    pop hl
    ret

release:
    ld a,(_nmi_phase)
    or a
    jr z,wrong_state
; Also called when the outer ADL application returns to MOS.
_nmi_app_leave:
    ld a,(_nmi_phase)
    cp GUEST                    ; All armed phases are >= GUEST.
    jp nc,_nmi_reset
    xor a
    ld (_nmi_phase),a
    ret

run_guest:
    ld a,(_nmi_phase)
    cp CLAIMED
    jr nz,wrong_state
    call upper_hl
    cp 5
    jr c,invalid
    cp 11
    jr nc,invalid
    ld (guest_bank),a
    jr enter_guest

resume_guest:
    ld a,(_nmi_phase)
    cp PARKED
    jr nz,wrong_state
enter_guest:
    di
    ; Keep the public caller's register state on SPL while the guest runs.
    push bc
    push de
    push hl
    push ix
    push iy
    exx
    push bc
    push de
    push hl
    exx
    ex af,af'
    push af
    ex af,af'
    ld (host_spl),sp
    ld a,(_nmi_phase)
    cp PARKED
    jr z,resume_frame

    ; Make the initial three-byte Z80 frame on SPL; never touch SPS.
    ex de,hl
    dec sp
    dec sp
    dec sp
    ld hl,0
    add hl,sp
    ld (hl),2
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    stmix
    ld a,(guest_bank)
    ld mb,a
    ld a,0xc2                   ; SCLK 2^22, NMI.
    out0 (WDT_CTL),a
    ld a,GUEST
    ld (_nmi_phase),a
    retn.l

resume_frame:
    call reload_watchdog
    ld hl,-FRAME_LEN
    add hl,sp
    ld sp,hl
    ex de,hl
    ld hl,guest_frame
    ld bc,FRAME_LEN
    ldir
    ld a,(guest_bank)
    ld mb,a
    ld a,GUEST
    ld (_nmi_phase),a
    ex af,af'
    pop af
    ex af,af'
    exx
    pop hl
    pop de
    pop bc
    exx
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    retn.l

_nmi_dispatch:
    push af
    ld a,(_nmi_phase)
    or a
    jr z,default_nmi
    cp CLAIMED
    jr nz,check_guest
    in0 a,(WDT_CTL)
    bit 7,a
    jr z,default_nmi
    jp _nmi_reset
check_guest:
    cp GUEST
    jp nz,_nmi_reset
    ld a,i
    jp pe,_nmi_reset
    ld a,mb
    push hl
    ld hl,guest_bank
    cp (hl)
    pop hl
    jp nz,_nmi_reset

    push bc
    push de
    push hl
    push ix
    push iy
    exx
    push bc
    push de
    push hl
    exx
    ex af,af'
    push af
    ex af,af'
    ld ix,30
    add ix,sp
    ld a,(ix)
    cp 2
    jp nz,_nmi_reset

    ld hl,0
    add hl,sp
    ld de,guest_frame
    ld bc,FRAME_LEN
    ldir
    ld a,CALLBACK
    ld (_nmi_phase),a
    xor a
    ld mb,a
    ld sp,(host_spl)
    ; Leave the NMI before enabling the ordinary MOS interrupt handlers.
    ld hl,host_continuation
    push hl
    dec sp
    ld hl,0
    add hl,sp
    ld (hl),3
    di
    retn.l

default_nmi:
    pop af
    jp __default_nmi_handler

host_continuation:
    call reload_watchdog
    ld ix,(callback_address)
    ld de,(callback_context)
    ld hl,(guest_frame+31)      ; PC16 followed by guest_bank in BSS.
    ld a,1
    ei
    call invoke_callback
    di
    ld a,PARKED
    ld (_nmi_phase),a
    ex af,af'
    pop af
    ex af,af'
    exx
    pop hl
    pop de
    pop bc
    exx
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    xor a
    ei
    ret

invoke_callback:
    jp (ix)

_nmi_reset:
    di
    ld a,RECOVERY
    ld (_nmi_phase),a
_nmi_reset_early:
    xor a
    ld mb,a
    ld a,0x83                   ; Armed reset, SCLK 2^18.
    out0 (WDT_CTL),a
    ld a,0xa5
    out0 (WDT_RELOAD),a
    ld a,0x5a
    out0 (WDT_RELOAD),a
reset_wait:
    halt
    jr reset_wait

reload_watchdog:
    ld a,0xa5
    out0 (WDT_RELOAD),a
    ld a,0x5a
    out0 (WDT_RELOAD),a
    ret

.bss
; These two labels are adjacent so the foreground gets the full guest PC.
guest_frame: .ds FRAME_LEN
guest_bank: .ds 1
host_spl: .ds 3
callback_address: .ds 3
callback_context: .ds 3
_nmi_phase: .ds 1
_nmi_app_context: .ds 1
