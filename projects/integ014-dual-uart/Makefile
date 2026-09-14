NAME = DUALUART
LDHAS_ARG_PROCESSING = 1
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -std=c17 -Wall -Wextra -Werror -Ibuild
obj/main.o: src/main.c build/firmware_abi.h
obj/entries.o: src/entries.asm build/firmware_abi.inc
