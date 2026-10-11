; PORT-008 STREAM-TIMING-R01 only. Same native kernel and GPIO ownership;
; one PRT snapshot after each successful forward block, before cleanup.
; No stable public ABI. Remove this composition after timing investigation.
EMOS_STREAM_TIMING_IO EQU 1
            INCLUDE "emos_parallel_native_body.inc"
