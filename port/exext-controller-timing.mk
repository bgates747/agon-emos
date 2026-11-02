# PORT-008 resumption: matched format4 timing with the current controller endpoint.
# The owning profiles retain all link/ROM guards and stock-shaped input handling.
# No production selection or startup/test invocation is performed here.
include $(dir $(lastword $(MAKEFILE_LIST)))exext-stream-timing.mk
include $(dir $(lastword $(MAKEFILE_LIST)))controller-options.mk
