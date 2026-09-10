# Non-release fixed-backend composition for PORT-008-D002. Its procedure makes
# the explicit mode request only after the operator/top-level has prepared the
# peer; EMOS has no external attestation channel. This profile adds no
# activation, General Poll, return transport, application command, or release
# identity to its parallel procedure. Shared console objects keep resident EMOS
# linkable; this held profile is not console qualification. Firmware and
# qualification-composition identities remain separate.
include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk
C_SOURCES_EXTRA := \
	src/emos_console.c \
	src/emos_keyboard.c \
	src/emos_uart_flow.c \
	src/emos_uart_probe.c \
	src/emos.c \
	src/emos_parallel.c \
	src/emos_parallel_engine.c \
	src/emos_parallel_fixed_backend.c
C_OBJECT_RELATIVE_EXTRA := \
	src/emos_console.o \
	src/emos_keyboard.o \
	src/emos_uart_flow.o \
	src/emos_uart_probe.o \
	src/emos.o \
	src/emos_parallel.o \
	src/emos_parallel_engine.o \
	src/emos_parallel_fixed_backend.o
ASM_SOURCES_EXTRA := src/emos_console_io.asm src/emos_keyboard_io.asm src/emos_parallel_io.asm
ASM_OBJECT_RELATIVE_EXTRA := src/emos_console_io.o src/emos_keyboard_io.o src/emos_parallel_io.o
C_SOURCE_CPPFLAGS_RELATIVE := src/emos.c
C_SOURCE_CPPFLAGS_EXTRA := \
	$(EMOS_IDENTITY_CPPFLAGS) \
	-DEMOS_QUALIFICATION_COMPOSITION_IDENTITY=\\\"$(EMOS_QUALIFICATION_COMPOSITION_IDENTITY)\\\" \
	-DEMOS_PARALLEL_FIXED_QUALIFICATION=1
PARITY_EXPECTED_COMMANDS := EMOS
FIRMWARE_LINK_CHECKS := \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_parallel_fixed.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_keyboard.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_console.py
