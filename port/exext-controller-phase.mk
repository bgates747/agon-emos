# PORT-008 boundary latency: existing RAM phase hooks plus controller endpoint.
# Diagnostic-only composition; ordinary profiles have no active phase calls.
# No native byte-loop timestamp, transport/scheduling change or startup action.
include $(dir $(lastword $(MAKEFILE_LIST)))exext-phase-timing.mk
include $(dir $(lastword $(MAKEFILE_LIST)))controller-options.mk
