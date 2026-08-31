# Fixed-purpose EMOS build profile for agon-extender PORT-008's r01
# forward-only prototype. This profile is never the default EMOS product build.
include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk
C_SOURCES_EXTRA := src/emos.c
C_OBJECT_RELATIVE_EXTRA := src/emos.o
CPPFLAGS_EXTRA := $(EMOS_IDENTITY_CPPFLAGS) -DEMOS_PORT008_FORWARD=1
PARITY_EXPECTED_COMMANDS := EMOS
FIRMWARE_LINK_CHECKS := $(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py
