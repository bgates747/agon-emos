# ----------------------------
# Makefile Options
# ----------------------------

NAME=mos
LDHAS_EXIT_HANDLER=0
LDHAS_ARG_PROCESSING=0

include makefile.inc

# EMOS-001 native product wrapper. Keep every imported mandatory linked check.
PYTHON ?= python3
FIRMWARE_LINK_CHECKS := verify_startup.py verify_uart_baud.py verify_parallel_upload.py verify_keyboard.py verify_console.py
ifeq ($(EMOS_UART0_RFC25),1)
FIRMWARE_LINK_CHECKS += verify_uart0_rfc25.py
ifneq ($(EMOS_NETWORK_PARALLEL),1)
FIRMWARE_LINK_CHECKS += verify_uart_nm.py
endif
endif

ifeq ($(EMOS_UART1_APP),1)
FIRMWARE_LINK_CHECKS += verify_neta.py
endif

$(BINDIR)/$(NAME).elf: $(OBJS)
	$(LINKER) $(LINKER_ARG) $(LINKER_EXIT) -Map=$(BINDIR)/$(NAME).elf.map -T $(LINKERCONFIG) -o $@ $(OBJS) $(LINKERLIBFLAGS)

.PHONY: firmware-check
firmware-check: all $(BINDIR)/$(NAME).elf
	@set -e; for check in $(FIRMWARE_LINK_CHECKS); do $(PYTHON) projects/emos/$$check $(if $(filter 1,$(EMOS_NETWORK_PARALLEL)),$$(test "$$check" != verify_parallel_upload.py || echo --network),) --source . --elf $(BINDIR)/$(NAME).elf --nm $(TOOLBINDIR)/ez80-none-elf-nm --objdump $(TOOLBINDIR)/ez80-none-elf-objdump; done
	$(PYTHON) projects/emos/verify_control_tx.py $(if $(filter 1,$(EMOS_CONTROL_TX_DIAGNOSTIC)),--enabled,) --source . --elf $(BINDIR)/$(NAME).elf --nm $(TOOLBINDIR)/ez80-none-elf-nm

# Optional generated identity is scoped to the only identity-owning C unit.
ifneq ($(strip $(EMOS_IDENTITY_HEADER)),)
$(OBJDIR)/emos.o: CFLAGS += -include $(EMOS_IDENTITY_HEADER)
$(OBJDIR)/emos.o: $(EMOS_IDENTITY_HEADER)
endif
