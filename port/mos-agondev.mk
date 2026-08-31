# Explicit product profile consumed by mos-agondev. EMOS owns these additions;
# the generic port must not hard-code them.
include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk
C_SOURCES_EXTRA := src/emos.c
C_OBJECT_RELATIVE_EXTRA := src/emos.o
CPPFLAGS_EXTRA := $(EMOS_IDENTITY_CPPFLAGS)
PARITY_EXPECTED_COMMANDS := EMOS
