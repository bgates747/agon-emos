# PORT-008 LC03-A: private MOSlet qualification service, never production.
include $(dir $(lastword $(MAKEFILE_LIST)))parallel-native-candidate.mk
CPPFLAGS_EXTRA += -DEMOS_PARALLEL_PROBE=1
FIRMWARE_LINK_CHECKS := $(subst /verify_parallel_native.py,/verify_parallel_probe.py,$(FIRMWARE_LINK_CHECKS))
