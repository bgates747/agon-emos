NAME = U1SEND
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -Wall -Wextra -Werror -Ibuild
obj/main.o: src/main.c build/uart_forward_probe_config.h
