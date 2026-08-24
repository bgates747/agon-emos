TOOLCHAIN ?= $(abspath ../../toolchains/agondev)
PYTHON ?= $(abspath ../../.venv/bin/python)

TOOLBIN := $(TOOLCHAIN)/bin
TOOLLIB := $(TOOLCHAIN)/lib
CC := $(TOOLBIN)/ez80-none-elf-clang
AS := $(TOOLBIN)/ez80-none-elf-as
LD := $(TOOLBIN)/ez80-none-elf-ld
OBJCOPY := $(TOOLBIN)/ez80-none-elf-objcopy
NM := $(TOOLBIN)/ez80-none-elf-nm

CFLAGS := -target ez80-none-elf -march=ez80 \
	-mllvm -z80-gas-style -mllvm -z80-print-zero-offset \
	-std=c17 -Wall -Wextra -Werror -Oz -ffreestanding -fno-builtin \
	-fno-threadsafe-statics -Wa,-march=ez80+full
ASFLAGS := -march=ez80+full+adl
LDFLAGS := --orphan-handling=error -T ld/provider.ld

VALID := build/media/valid
CASES := build/media/cases

.DEFAULT_GOAL := all

.PHONY: all media validate clean
all: $(VALID)/hello.emo $(VALID)/echo.emo $(VALID)/edu-probe.emo

build/hello.o: src/hello.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/echo.o: src/echo.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.elf: build/%.o ld/provider.ld
	$(LD) $(LDFLAGS) -Map $(@:.elf=.map) $< -L$(TOOLLIB) -l agon -o $@
	@test "$$($(NM) -n $@ | awk '$$3 == "_emos_provider_entry" { print $$1 }')" = "000b0080"

build/%.bin: build/%.elf
	$(OBJCOPY) -O binary $< $@

$(VALID)/hello.emo: manifests/hello.json build/hello.bin emos_module.py
	@mkdir -p $(dir $@)
	$(RM) $@
	$(PYTHON) -B emos_module.py build $< build/hello.bin $@

$(VALID)/echo.emo: manifests/echo.json build/echo.bin emos_module.py
	@mkdir -p $(dir $@)
	$(RM) $@
	$(PYTHON) -B emos_module.py build $< build/echo.bin $@

$(VALID)/edu-probe.emo: manifests/edu-probe.json build/echo.bin emos_module.py
	@mkdir -p $(dir $@)
	$(RM) $@
	$(PYTHON) -B emos_module.py build $< build/echo.bin $@

media: all build_media.py
	$(RM) -r $(CASES)
	$(PYTHON) -B build_media.py $(VALID) $(CASES)

validate: all media
	$(PYTHON) -B emos_module.py validate-dir $(VALID)

clean:
	$(RM) -r build
