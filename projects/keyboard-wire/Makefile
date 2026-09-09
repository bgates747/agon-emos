NAME = KBWIRE
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -Wall -Wextra -Werror -Ibuild
obj/main.o: src/main.c build/build_identity.h
