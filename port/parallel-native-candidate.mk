# PORT-008 F02c3 private build probe ONLY. No live payload caller or mode API.
# A ROM overflow must fail the ordinary linker assertion; do not relax it.
include $(dir $(lastword $(MAKEFILE_LIST)))parallel-boot-candidate.mk
C_SOURCES_EXTRA += src/emos_parallel_native.c src/emos_parallel_coordinator.c
C_OBJECT_RELATIVE_EXTRA += src/emos_parallel_native.o src/emos_parallel_coordinator.o
ASM_SOURCES_EXTRA += src/emos_parallel_native_io.asm
ASM_OBJECT_RELATIVE_EXTRA += src/emos_parallel_native_io.o
# Reuse every ordinary check; only the parallel inventory admits this
# private raw leaf, with its guarded/no-live-caller and atomic-helper checks.
FIRMWARE_LINK_CHECKS := $(subst /verify_parallel.py,/verify_parallel_native.py,$(FIRMWARE_LINK_CHECKS))

# Native coordination affects only the private startup owner; ordinary/boot-only
# compositions retain their exact previous reset monitor and caller inventory.
CPPFLAGS_EXTRA += -DEMOS_PARALLEL_NATIVE_COORDINATOR=1
