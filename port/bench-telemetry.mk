# UARTFLOW is now an SD MOSlet; keep its small resident transport service.
# This remains a separate non-release telemetry composition.
include $(dir $(lastword $(MAKEFILE_LIST)))mos-agondev.mk
CPPFLAGS_EXTRA += -DEMOS_BENCH_TELEMETRY=1
