; PORT-008 PHASE-TIMING-R01 private diagnostic leaf, no ordinary residency.
; Armed only by the exact-ROM-bound MOSlet while it owns PRT1 and RAM recorder.
; B0020 is its verified private jump vector inside the MOSlet header padding.
; Aliases share bytes; final caller PCs are decoded from original relocations.
; Disabled and enabled paths preserve registers/flags/IFF. Reset clears armed.
            .ASSUME ADL = 1
            DEFINE .STARTUP, SPACE = ROM
            SEGMENT .STARTUP
            XREF _emos_phase_armed
            XDEF _emos_phase_reserve_begin, _emos_phase_reserve_end
            XDEF _emos_phase_offer_begin, _emos_phase_coordinate_begin
            XDEF _emos_phase_park_end, _emos_phase_payload_begin, _emos_phase_payload_end
            XDEF _emos_phase_unpark_end, _emos_phase_completion_begin, _emos_phase_completion_end
            XDEF _emos_phase_service_begin, _emos_phase_service_end
            XDEF _emos_phase_poll_begin, _emos_phase_poll_end
            XDEF _emos_phase_publish_begin, _emos_phase_publish_end
_emos_phase_reserve_begin:
_emos_phase_reserve_end:
_emos_phase_offer_begin:
_emos_phase_coordinate_begin:
_emos_phase_park_end:
_emos_phase_payload_begin:
_emos_phase_payload_end:
_emos_phase_unpark_end:
_emos_phase_completion_begin:
_emos_phase_completion_end:
_emos_phase_service_begin:
_emos_phase_service_end:
_emos_phase_poll_begin:
_emos_phase_poll_end:
_emos_phase_publish_begin:
_emos_phase_publish_end:
            PUSH AF
            LD A, (_emos_phase_armed)
            OR A, A
            JR Z, phase_disabled
            POP AF
            JP 0B0020h
phase_disabled:
            POP AF
            RET
            END
