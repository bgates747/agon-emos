# PORT-008 F02c3 private build probe ONLY. No live payload caller or mode API.
# A ROM overflow must fail the ordinary linker assertion; do not relax it.
include $(dir $(lastword $(MAKEFILE_LIST)))parallel-boot-candidate.mk
C_SOURCES_EXTRA += src/emos_parallel_native.c
C_OBJECT_RELATIVE_EXTRA += src/emos_parallel_native.o
ASM_SOURCES_EXTRA += src/emos_parallel_native_io.asm
ASM_OBJECT_RELATIVE_EXTRA += src/emos_parallel_native_io.o
