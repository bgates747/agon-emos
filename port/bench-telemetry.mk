# BENCH-001 non-release composition. Full EMOS has only 153 bytes of spare ROM
# before telemetry. Replace the old standalone UARTFLOW diagnostic in this
# explicitly selected image; preserve keyboard, SD, console and all stock MOS.
# The ordinary source profile retains UARTFLOW and does not admit telemetry.
include $(dir $(lastword $(MAKEFILE_LIST)))mos-agondev.mk
C_SOURCES_EXTRA := $(filter-out src/emos_uart_flow.c,$(C_SOURCES_EXTRA))
C_OBJECT_RELATIVE_EXTRA := $(filter-out src/emos_uart_flow.o,$(C_OBJECT_RELATIVE_EXTRA))
CPPFLAGS_EXTRA += -DEMOS_BENCH_TELEMETRY=1
