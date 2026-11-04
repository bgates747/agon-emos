; EMOS-001 maintained native port from EMOS6ececd56; initial dialect conversion
; by mos-agondev zds2gas. Review/source changes belong here, not generated trees.
;
; one PRT snapshot after each successful forward block, before cleanup.
; No stable public ABI. Remove this composition after timing investigation.
.equiv EMOS_STREAM_TIMING_IO, 1
            .include "emos_parallel_native_body.inc"
