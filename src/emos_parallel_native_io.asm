; EMOS-001: maintained native import from EMOS6ececd56.
; Initial dialect conversion by mos-agondev; review/edit this source directly.
; Normal native payload: shared body, timing instrumentation assembled out.
; PORT-008 STREAM-TIMING-R01 has a separate explicit wrapper/profile.
.equiv EMOS_STREAM_TIMING_IO, 0
            .include "emos_parallel_native_body.inc"
