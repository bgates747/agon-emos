# PORT-008 STREAM-TIMING-R01: explicit measurement-only composition.
# Normal ExExt remains byte-identical at the native kernel; this profile adds
# one PRT snapshot per forward block and a private guarded zero-length flush.
include $(dir $(lastword $(MAKEFILE_LIST)))exext-stream-candidate.mk
CPPFLAGS_EXTRA += -DEMOS_STREAM_TIMING=1
ASM_SOURCES_EXTRA := $(filter-out src/emos_parallel_native_io.asm,$(ASM_SOURCES_EXTRA)) src/emos_parallel_native_timing_io.asm src/emos_stream_timing_io.asm
ASM_OBJECT_RELATIVE_EXTRA := $(filter-out src/emos_parallel_native_io.o,$(ASM_OBJECT_RELATIVE_EXTRA)) src/emos_parallel_native_timing_io.o src/emos_stream_timing_io.o
