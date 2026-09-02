# Explicit product profile consumed by mos-agondev. EMOS owns these additions;
# the generic port must not hard-code them.
include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk
C_SOURCES_EXTRA := src/emos.c src/emos_parallel.c src/emos_parallel_engine.c
C_OBJECT_RELATIVE_EXTRA := src/emos.o src/emos_parallel.o src/emos_parallel_engine.o
ASM_SOURCES_EXTRA := src/emos_parallel_io.asm
ASM_OBJECT_RELATIVE_EXTRA := src/emos_parallel_io.o
C_SOURCE_CPPFLAGS_RELATIVE := src/emos.c
C_SOURCE_CPPFLAGS_EXTRA := $(EMOS_IDENTITY_CPPFLAGS)
PARITY_EXPECTED_COMMANDS := EMOS
FIRMWARE_LINK_CHECKS := \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_parallel.py
