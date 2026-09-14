# Explicit product profile consumed by mos-agondev. EMOS owns these additions;
# the generic port must not hard-code them.
include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk
C_SOURCES_EXTRA := src/emos_telemetry.c src/emos_sdlink.c src/emos_console.c src/emos_keyboard.c src/emos_uart_flow.c src/emos_uart_probe.c src/emos.c src/emos_parallel.c src/emos_parallel_engine.c
C_OBJECT_RELATIVE_EXTRA := src/emos_telemetry.o src/emos_sdlink.o src/emos_console.o src/emos_keyboard.o src/emos_uart_flow.o src/emos_uart_probe.o src/emos.o src/emos_parallel.o src/emos_parallel_engine.o
ASM_SOURCES_EXTRA := src/emos_console_io.asm src/emos_keyboard_io.asm src/emos_parallel_io.asm
ASM_OBJECT_RELATIVE_EXTRA := src/emos_console_io.o src/emos_keyboard_io.o src/emos_parallel_io.o
C_SOURCE_CPPFLAGS_RELATIVE := src/emos.c
C_SOURCE_CPPFLAGS_EXTRA := $(EMOS_IDENTITY_CPPFLAGS)
PARITY_EXPECTED_COMMANDS := EMOS
PARITY_EXPECTED_BOOT_LINE := EMOS identity: $(EMOS_SOURCE_IDENTITY), build $(EMOS_BUILD_ID), status $(EMOS_ARTIFACT_STATUS)
FIRMWARE_LINK_CHECKS := \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_parallel.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_keyboard.py \
	$(EMOS_PROFILE_ROOT)/projects/emos/verify_console.py
