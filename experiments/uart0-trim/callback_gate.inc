; Optional app-side wrapper for one known C callback. No mutable dispatch slot.
; Instantiate once for each C callback used by the application.
.macro protect_callback gate,body
\gate:
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
 call \body
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
 ret
.endm
