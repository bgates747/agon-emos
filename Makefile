PYTHON ?= python3
MOS_AGONDEV_ROOT ?= ../mos-agondev
AGONDEV_TOOLCHAIN ?= $(MOS_AGONDEV_ROOT)/toolchains/agondev
FAB_ROOT ?= $(MOS_AGONDEV_ROOT)/fab-agon-emulator
SOURCE_PROFILE := $(abspath port/mos-agondev.mk)

.DEFAULT_GOAL := help

.PHONY: help test modules linked-check firmware-check qualify

help:
	@echo "test           run repository-local Python tests"
	@echo "modules        build and validate EMOS provider fixtures"
	@echo "linked-check   verify EMOS ABI and VDU dispatch in the built image"
	@echo "firmware-check build EMOS through the configured mos-agondev checkout"
	@echo "qualify        run mos-agondev's complete configured-input gate"

test:
	$(PYTHON) -m unittest discover -s tests -v

modules:
	$(MAKE) -C projects/emos TOOLCHAIN=$(abspath $(AGONDEV_TOOLCHAIN)) \
		PYTHON=$(PYTHON) validate

linked-check:
	$(PYTHON) -B projects/emos/verify_abi.py \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--binary $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.bin \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm
	$(PYTHON) -B projects/emos/verify_vdu.py \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--binary $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.bin \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm

firmware-check:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) SOURCE_PROFILE=$(SOURCE_PROFILE) firmware-check
	$(MAKE) linked-check

qualify:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) SOURCE_PROFILE=$(SOURCE_PROFILE) \
		FAB_ROOT=$(abspath $(FAB_ROOT)) verify
	$(MAKE) linked-check
	$(MAKE) test
	$(MAKE) modules
