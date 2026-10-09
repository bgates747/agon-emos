# PORT-008 private non-release startup composition; no payload activation.
# Retain all ordinary product link checks and swap only the boot hook owner.
include $(dir $(lastword $(MAKEFILE_LIST)))mos-agondev.mk
C_SOURCES_EXTRA := $(filter-out src/emos_parallel_startup_disabled.c,$(C_SOURCES_EXTRA)) src/emos_parallel_startup.c
C_OBJECT_RELATIVE_EXTRA := $(filter-out src/emos_parallel_startup_disabled.o,$(C_OBJECT_RELATIVE_EXTRA)) src/emos_parallel_startup.o
