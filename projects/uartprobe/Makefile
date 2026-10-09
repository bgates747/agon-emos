# One maintained algorithm/adapter set, two compatible EMOS command names.
NAME ?= uarttest
ifeq ($(NAME),uarttest)
PROBE_RUN = emos_uart_probe
else ifeq ($(NAME),vdppoll)
PROBE_RUN = emos_general_poll
else
$(error NAME must be uarttest or vdppoll)
endif
RAM_START = 0xB0000
RAM_SIZE = 0x8000
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
override OBJDIR := obj/$(NAME)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -Wall -Wextra -Werror -I../uartflow/src -DPROBE_RUN=$(PROBE_RUN) -DPROBE_NAME='"$(NAME)"'
OBJS += $(OBJDIR)/gateway.o $(OBJDIR)/emos_gateway.o
$(LINKBINARY): $(OBJDIR)/gateway.o $(OBJDIR)/emos_gateway.o
$(OBJDIR)/gateway.o: ../uartflow/src/gateway.c ../uartflow/src/flow.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -c -o $@
$(OBJDIR)/emos_gateway.o: ../uartflow/src/emos_gateway.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@
$(OBJDIR)/probe.o: ../../src/emos_probe_clock.h ../uartflow/src/flow.h src/probe.h
$(OBJDIR)/main.o: src/probe.h
