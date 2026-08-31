TOOLCHAIN ?= $(abspath ../../../mos-agondev/toolchains/agondev)
PYTHON ?= python3

TOOLBIN := $(TOOLCHAIN)/bin
AS := $(TOOLBIN)/ez80-none-elf-as
LD := $(TOOLBIN)/ez80-none-elf-ld
OBJCOPY := $(TOOLBIN)/ez80-none-elf-objcopy
NM := $(TOOLBIN)/ez80-none-elf-nm
BUILD := build
SOURCE_ROOT := $(abspath ../..)

.DEFAULT_GOAL := validate

.PHONY: validate clean

$(BUILD)/fixture.asm: fixture.json build_fixture.py
	$(PYTHON) -B build_fixture.py fixture.json $@

$(BUILD)/fixture.o: $(BUILD)/fixture.asm
	$(AS) -march=ez80+full+adl $< -o $@

$(BUILD)/P8VDU.elf: $(BUILD)/fixture.o fixture.ld
	$(LD) --orphan-handling=error -T fixture.ld -Map $(BUILD)/P8VDU.map $< -o $@

$(BUILD)/P8VDU.BIN: $(BUILD)/P8VDU.elf
	$(OBJCOPY) -O binary $< $@

validate: $(BUILD)/P8VDU.BIN
	$(PYTHON) -B verify.py --source $(SOURCE_ROOT) --manifest fixture.json \
		--binary $(BUILD)/P8VDU.BIN --elf $(BUILD)/P8VDU.elf --nm $(NM)

clean:
	$(RM) -r $(BUILD)
