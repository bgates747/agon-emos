# verbosity - comment to show all output
#V = @

# Tool directories
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
TOOLBINDIR=$(AGONDEV_TOOLCHAIN)/bin
INCLUDEDIR=$(AGONDEV_TOOLCHAIN)/include
LINKERCONFIG=linker.conf
LIBDIR=$(AGONDEV_TOOLCHAIN)/lib

# Architecture
ARCH=ez80+full
TARGET=ez80-none-elf

# Tools and flags
## Compiler
CC=$(TOOLBINDIR)/ez80-none-elf-clang
CFLAGS=-mllvm -z80-gas-style -mllvm -z80-print-zero-offset -nostdinc -target $(TARGET) -Oz -Wa,-march=$(ARCH) -Wextra -Wno-unused-parameter -I src_fatfs -I src_umm_malloc -I src -I $(INCLUDEDIR)
## Assembler
ASM=$(TOOLBINDIR)/ez80-none-elf-as
ASMFLAGS=-march=$(ARCH) -I src
ifeq ($(EMOS_CONTROL_TX_DIAGNOSTIC),1)
ASMFLAGS += --defsym EMOS_CONTROL_TX_DIAGNOSTIC=1
endif
## Linker
LINKER=$(TOOLBINDIR)/ez80-none-elf-ld
ifeq ($(LDHAS_ARG_PROCESSING),1)
	LINKER_ARG=-defsym=_parse_option=___arg_processing
endif
ifeq ($(LDHAS_EXIT_HANDLER), 1)
    LINKER_EXIT=-defsym=_has_exit_handler=1
else
    LINKER_EXIT=-defsym=_has_exit_handler=0
endif
LINKERFLAGS=$(LINKER_ARG) $(LINKER_EXIT) -Map=$(BINDIR)/$(NAME).map -T $(LINKERCONFIG) --oformat binary -o
LINKERLIBFLAGS=-L$(LIBDIR) -lagon
SETPROGNAME=$(TOOLBINDIR)/agondev-setname

# project directories
SRCDIR=src
OBJDIR=obj
BINDIR=bin
# Final binary
BINARY=$(BINDIR)/$(NAME).bin

# Automatically get all object names from sourcefiles
ALL_OBJS=$(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.asm)) \
	 $(patsubst $(SRCDIR)/%.s,   $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.s)) \
	 $(patsubst $(SRCDIR)/%.src, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.src)) \
	 $(patsubst src_fatfs/%.c,   src_fatfs/%.o, $(wildcard src_fatfs/*.c)) \
	 $(patsubst src_umm_malloc/%.c,   src_umm_malloc/%.o, $(wildcard src_umm_malloc/*.c)) \
	 $(patsubst $(SRCDIR)/%.c,   $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.c)) \
	 $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cpp))

# EMOS-001 explicit resident composition: no automatic inclusion of experiments.
EMOS_C := emos emos_admission emos_telemetry emos_sdlink emos_console emos_keyboard emos_uart_flow emos_parallel emos_parallel_engine emos_parallel_handover emos_parallel_boot emos_parallel_startup emos_controller emos_net emos_parallel_upload
EMOS_ASM := emos_console_io emos_keyboard_io emos_parallel_io emos_parallel_upload_io
# NET-004 optional carrier; explicit binding must supply all link_* functions.
ifeq ($(EMOS_NETWORK_PARALLEL),1)
EMOS_C := $(filter-out emos_net,$(EMOS_C))
EMOS_C += emos_app_span emos_netp emos_netp_link emos_parallel_coordinator emos_parallel_native
EMOS_ASM += emos_parallel_native_io
CFLAGS += -DEMOS_NETWORK_PARALLEL
endif

# INTEG-021 replaces the historical trial only in the explicit paired profile.
ifeq ($(EMOS_UART0_RFC25),1)
ifneq ($(EMOS_NETWORK_PARALLEL),1)
# INTEG-023: the guarded UART0 owner also supports the explicit UART1-NM
# composition. Link the common application-span contract exactly once.
EMOS_C += emos_app_span
endif
ALL_OBJS := $(filter-out $(OBJDIR)/uart0_async.o,$(ALL_OBJS))
EMOS_C += emos_uart0_rfc25
EMOS_ASM += emos_uart0_rfc25_io
CFLAGS += -DEMOS_UART0_RFC25=1
ASMFLAGS += --defsym EMOS_UART0_RFC25=1
else
ASMFLAGS += --defsym EMOS_UART0_RFC25=0
endif

# INTEG-024 explicit app engine owner glue. No implicit old-profile change.
ifeq ($(EMOS_UART1_APP),1)
ifneq ($(EMOS_UART0_RFC25),1)
$(error EMOS_UART1_APP requires the guarded UART0/UART1-NM composition)
endif
ifeq ($(EMOS_NETWORK_PARALLEL),1)
$(error EMOS_UART1_APP cannot use the parallel network composition)
endif
CFLAGS += -DEMOS_UART1_APP
ASMFLAGS += --defsym EMOS_UART1_APP=1
EMOS_C += emos_neta
endif

# Diagnostic private OPEN7 / app marker bridge, never an ordinary profile default.
ifeq ($(EMOS_UART1_RX_DIAGNOSTIC),1)
ifneq ($(EMOS_UART1_APP),1)
$(error EMOS_UART1_RX_DIAGNOSTIC requires EMOS_UART1_APP)
endif
CFLAGS += -DEMOS_UART1_RX_DIAGNOSTIC=1
ASMFLAGS += --defsym EMOS_UART1_RX_DIAGNOSTIC=1
EMOS_ASM += emos_neta_diag_io
endif

OBJS = $(filter-out $(OBJDIR)/emos%.o,$(ALL_OBJS)) $(addprefix $(OBJDIR)/,$(addsuffix .o,$(EMOS_C) $(EMOS_ASM)))
CFLAGS += -DEMOS_CONTROLLER_INPUT=1 -DEMOS_PARALLEL_UPLOAD=1 -DEMOS_PARALLEL_NATIVE_COORDINATOR=1
# Source/build identities remain fail-closed exploratory values. Not deployable.

# Default rule
all: $(BINDIR) $(OBJDIR) $(BINARY)

# Linking all compiled objects into final binary
$(BINARY):$(OBJS)
	@echo [Linking $(BINARY)]
	$(V)$(LINKER) $(LINKERFLAGS)$@ $(OBJS) $(LINKERLIBFLAGS)

# Compile each .c file into .o file
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(V)$(CC) $(CFLAGS) $< -c -o $@
# Compile each .cpp file into .o file
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(V)$(CC) $(CFLAGS) $< -c -o $@
# Assemble each .s file into .o file
$(OBJDIR)/%.o: $(SRCDIR)/%.s
	$(V)$(ASM) $(ASMFLAGS) $< -o $@
# Assemble each .asm file into .o file
$(OBJDIR)/%.o: $(SRCDIR)/%.asm
	$(V)$(ASM) $(ASMFLAGS) $< -o $@
# Assemble each .src file into .o file
$(OBJDIR)/%.o: $(SRCDIR)/%.src
	$(V)$(ASM) $(ASMFLAGS) $< -o $@

# Directories to create
$(BINDIR):
	@mkdir $(BINDIR)
$(OBJDIR):
	@mkdir $(OBJDIR)

clean:
	@$(RM) -r $(BINDIR) $(OBJDIR)
	@$(RM) src_fatfs/*.o src_umm_malloc/*.o

