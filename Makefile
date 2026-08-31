PYTHON ?= python3
MOS_AGONDEV_ROOT ?= ../mos-agondev
MOS_WORKTREE ?= $(MOS_AGONDEV_ROOT)/projects/mos-port/worktree
AGONDEV_TOOLCHAIN ?= $(MOS_AGONDEV_ROOT)/toolchains/agondev
FAB_ROOT ?= $(MOS_AGONDEV_ROOT)/fab-agon-emulator
SOURCE_PROFILE := $(abspath port/mos-agondev.mk)
PORT008_SOURCE_PROFILE := $(abspath port/port008-forward.mk)

.DEFAULT_GOAL := help

.PHONY: help test modules linked-check contract-linked-check uart-baud-check firmware-check port008-fixture \
	port008-linked-check port008-firmware-check qualify

help:
	@echo "test           run repository-local Python tests"
	@echo "modules        build and validate EMOS provider fixtures"
	@echo "linked-check   verify EMOS ABI and VDU dispatch in the built image"
	@echo "firmware-check build EMOS through the configured mos-agondev checkout"
	@echo "port008-fixture build and verify the PORT-008 cold-boot fixture"
	@echo "port008-linked-check inspect the linked PORT-008 GPIO sender"
	@echo "port008-firmware-check build and inspect the fixed PORT-008 profile"
	@echo "qualify        run mos-agondev's complete configured-input gate"

test:
	MOS_AGONDEV_WORKTREE="$(abspath $(MOS_WORKTREE))" \
		$(PYTHON) -m unittest discover -s tests -v

modules:
	$(MAKE) -C projects/emos TOOLCHAIN=$(abspath $(AGONDEV_TOOLCHAIN)) \
		PYTHON=$(PYTHON) validate

linked-check: contract-linked-check uart-baud-check

contract-linked-check:
	$(PYTHON) -B projects/emos/verify_abi.py \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--binary $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.bin \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm
	$(PYTHON) -B projects/emos/verify_vdu.py \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--binary $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.bin \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm

uart-baud-check:
	$(PYTHON) -B projects/emos/verify_uart_baud.py \
		--source . \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm \
		--objdump $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-objdump

firmware-check:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) SOURCE_PROFILE=$(SOURCE_PROFILE) firmware-check
	$(MAKE) contract-linked-check

port008-fixture:
	$(MAKE) -C projects/port008-forward \
		TOOLCHAIN=$(abspath $(AGONDEV_TOOLCHAIN)) PYTHON=$(PYTHON) validate

port008-linked-check: port008-fixture
	$(PYTHON) -B projects/port008-forward/verify.py \
		--source . --manifest projects/port008-forward/fixture.json \
		--emos-binary $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.bin \
		--emos-elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--emos-nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm \
		--emos-objdump $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-objdump

port008-firmware-check:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) \
		SOURCE_PROFILE=$(PORT008_SOURCE_PROFILE) firmware-check
	$(MAKE) port008-linked-check

qualify:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) SOURCE_PROFILE=$(SOURCE_PROFILE) \
		FAB_ROOT=$(abspath $(FAB_ROOT)) verify
	$(MAKE) contract-linked-check
	$(MAKE) test
	$(MAKE) modules
