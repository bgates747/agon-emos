# PORT-008 / INTEG-019 private file carrier, no production selection.
# Retain all existing native/ABI/divisor/pad checks; add sole bulk gateway/leaf.
# The old bounded native payload/coordinator are NOT linked: uploads use the
# retained receive leaf below. Reuse only their boot-owned block exclusion.
include $(dir $(lastword $(MAKEFILE_LIST)))parallel-boot-candidate.mk
include $(dir $(lastword $(MAKEFILE_LIST)))controller-options.mk
C_SOURCES_EXTRA += src/emos_parallel_upload.c
C_OBJECT_RELATIVE_EXTRA += src/emos_parallel_upload.o
ASM_SOURCES_EXTRA += src/emos_parallel_upload_io.asm
ASM_OBJECT_RELATIVE_EXTRA += src/emos_parallel_upload_io.o
CPPFLAGS_EXTRA += -DEMOS_PARALLEL_UPLOAD=1 -DEMOS_PARALLEL_NATIVE_COORDINATOR=1
FIRMWARE_LINK_CHECKS := $(subst /verify_parallel.py,/verify_parallel_upload.py,$(FIRMWARE_LINK_CHECKS))
