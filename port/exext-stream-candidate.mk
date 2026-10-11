# PORT-008 EX02: experimental ordinary ExExt, no production selection.
# The native coordinator replaces the obsolete indefinite forward epoch owner.
include $(dir $(lastword $(MAKEFILE_LIST)))parallel-native-candidate.mk
C_SOURCES_EXTRA := $(filter-out src/emos_parallel.c src/emos_parallel_engine.c,$(C_SOURCES_EXTRA)) src/emos_exext.c
C_OBJECT_RELATIVE_EXTRA := $(filter-out src/emos_parallel.o src/emos_parallel_engine.o,$(C_OBJECT_RELATIVE_EXTRA)) src/emos_exext.o
ASM_SOURCES_EXTRA += src/emos_exext_io.asm
ASM_OBJECT_RELATIVE_EXTRA += src/emos_exext_io.o
CPPFLAGS_EXTRA += -DEMOS_EXEXT_STREAM=1
FIRMWARE_LINK_CHECKS := $(subst /verify_parallel_native.py,/verify_exext.py,$(FIRMWARE_LINK_CHECKS))
