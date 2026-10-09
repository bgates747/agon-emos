# PORT-008 correctness-reference composition ONLY; never install as product.
# Ordinary/native profiles exclude this uncalled C receive loop to save ROM.
# Retain the complete ordinary wrapper and its mandatory linked checks.
include $(dir $(lastword $(MAKEFILE_LIST)))mos-agondev.mk
CPPFLAGS_EXTRA += -DEMOS_PARALLEL_RECEIVE_REFERENCE=1
