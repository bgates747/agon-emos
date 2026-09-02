# Non-release fixed-backend composition for PORT-008-D002. Its procedure makes
# the explicit mode request only after the operator/top-level has prepared the
# peer; EMOS has no external attestation channel. This profile adds no
# activation, General Poll, return transport, application command, or release
# identity. Firmware and qualification-composition identities remain separate.
include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk
C_SOURCES_EXTRA := \
	src/emos.c \
	src/emos_parallel.c \
	src/emos_parallel_engine.c \
	src/emos_parallel_fixed_backend.c
C_OBJECT_RELATIVE_EXTRA := \
	src/emos.o \
	src/emos_parallel.o \
	src/emos_parallel_engine.o \
	src/emos_parallel_fixed_backend.o
ASM_SOURCES_EXTRA := src/emos_parallel_io.asm
ASM_OBJECT_RELATIVE_EXTRA := src/emos_parallel_io.o
C_SOURCE_CPPFLAGS_RELATIVE := src/emos.c
C_SOURCE_CPPFLAGS_EXTRA := \
	$(EMOS_IDENTITY_CPPFLAGS) \
	-DEMOS_QUALIFICATION_COMPOSITION_IDENTITY=\\\"$(EMOS_QUALIFICATION_COMPOSITION_IDENTITY)\\\" \
	-DEMOS_PARALLEL_FIXED_QUALIFICATION=1
PARITY_EXPECTED_COMMANDS := EMOS
FIRMWARE_LINK_CHECKS := \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_parallel_fixed.py
