# Private MOSlet; all GPIO/transport decisions remain in EMOS API51h.
NAME = parprobe
RAM_START = 0xB0000
RAM_SIZE = 0x8000
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -Wall -Wextra -Werror
OBJS += $(OBJDIR)/emos_gateway.o
$(LINKBINARY): $(OBJDIR)/emos_gateway.o
$(OBJDIR)/emos_gateway.o: ../uartflow/src/emos_gateway.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@
$(OBJDIR)/main.o: src/pattern.h ../../src/emos_parallel_wire.h
