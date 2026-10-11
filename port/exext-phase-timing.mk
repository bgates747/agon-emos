# PORT-008 PHASE-TIMING-R01: RAM recorder, no transport behavior changes.
# No stream timing UART helper or native probe: the preceding matched campaign
# remains the UART control. Ordinary compositions preprocess all phase calls out.
include $(dir $(lastword $(MAKEFILE_LIST)))exext-stream-candidate.mk
CPPFLAGS_EXTRA += -DEMOS_PHASE_TIMING=1
ASM_SOURCES_EXTRA += src/emos_phase_timing_io.asm
ASM_OBJECT_RELATIVE_EXTRA += src/emos_phase_timing_io.o
