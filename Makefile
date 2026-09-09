PYTHON ?= python3
MOS_AGONDEV_ROOT ?= ../mos-agondev
MOS_AGONDEV_PYTHON ?= $(MOS_AGONDEV_ROOT)/.venv/bin/python
MOS_SOURCE ?= .
MOS_WORKTREE ?= $(MOS_AGONDEV_ROOT)/projects/mos-port/worktree
AGONDEV_TOOLCHAIN ?= $(MOS_AGONDEV_ROOT)/toolchains/agondev
FAB_ROOT ?= $(MOS_AGONDEV_ROOT)/fab-agon-emulator
PROVENANCE_DIR ?=
PROVENANCE_DIR_ABS := $(if $(strip $(PROVENANCE_DIR)),$(abspath $(PROVENANCE_DIR)))
SOURCE_PROFILE := $(abspath port/mos-agondev.mk)
PARALLEL_FIXED_SOURCE_PROFILE := $(abspath port/parallel-fixed-qualification.mk)

.DEFAULT_GOAL := help

.PHONY: help test modules linked-check contract-linked-check uart-baud-check \
	parallel-linked-check keyboard-linked-check firmware-check parallel-fixed-firmware-check \
	port008-fixture qualify

help:
	@echo "test           run repository-local Python tests"
	@echo "modules        build and validate EMOS provider fixtures"
	@echo "linked-check   verify EMOS ABI and VDU dispatch in the built image"
	@echo "firmware-check build EMOS through the configured mos-agondev checkout"
	@echo "parallel-fixed-firmware-check build the non-release fixed data-plane composition"
	@echo "PROVENANCE_DIR=PATH capture one fresh target-build provenance session"
	@echo "port008-fixture retain the superseded run's ordinary-VDU payload evidence"
	@echo "qualify        run mos-agondev's complete configured-input gate"

test:
	MOS_AGONDEV_WORKTREE="$(abspath $(MOS_WORKTREE))" \
		$(PYTHON) -m unittest discover -s tests -v

modules:
	$(MAKE) -C projects/emos TOOLCHAIN=$(abspath $(AGONDEV_TOOLCHAIN)) \
		PYTHON=$(PYTHON) validate

linked-check: contract-linked-check uart-baud-check parallel-linked-check keyboard-linked-check

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

parallel-linked-check:
	$(PYTHON) -B projects/emos/verify_parallel.py \
		--source . \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm \
		--objdump $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-objdump

keyboard-linked-check:
	$(PYTHON) -B projects/emos/verify_keyboard.py \
		--source . \
		--elf $(MOS_AGONDEV_ROOT)/projects/mos-port/bin/MOS.elf \
		--nm $(AGONDEV_TOOLCHAIN)/bin/ez80-none-elf-nm

firmware-check:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) \
		PYTHON="$(abspath $(MOS_AGONDEV_PYTHON))" \
		TOOLCHAIN="$(abspath $(AGONDEV_TOOLCHAIN))" \
		MOS_MAINTAINED_SOURCE="$(abspath $(MOS_SOURCE))" \
		MOS_WORKTREE="$(abspath $(MOS_WORKTREE))" \
		PROVENANCE_DIR="$(PROVENANCE_DIR_ABS)" \
		SOURCE_PROFILE=$(SOURCE_PROFILE) firmware-check
	$(MAKE) contract-linked-check

parallel-fixed-firmware-check:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) \
		PYTHON="$(abspath $(MOS_AGONDEV_PYTHON))" \
		TOOLCHAIN="$(abspath $(AGONDEV_TOOLCHAIN))" \
		MOS_MAINTAINED_SOURCE="$(abspath $(MOS_SOURCE))" \
		MOS_WORKTREE="$(abspath $(MOS_WORKTREE))" \
		PROVENANCE_DIR="$(PROVENANCE_DIR_ABS)" \
		SOURCE_PROFILE=$(PARALLEL_FIXED_SOURCE_PROFILE) firmware-check
	$(MAKE) contract-linked-check

port008-fixture:
	$(MAKE) -C projects/port008-forward \
		TOOLCHAIN=$(abspath $(AGONDEV_TOOLCHAIN)) PYTHON=$(PYTHON) validate

qualify:
	$(MAKE) -C $(MOS_AGONDEV_ROOT) \
		PYTHON="$(abspath $(MOS_AGONDEV_PYTHON))" \
		TOOLCHAIN="$(abspath $(AGONDEV_TOOLCHAIN))" \
		MOS_MAINTAINED_SOURCE="$(abspath $(MOS_SOURCE))" \
		MOS_WORKTREE="$(abspath $(MOS_WORKTREE))" \
		PROVENANCE_DIR="$(PROVENANCE_DIR_ABS)" \
		SOURCE_PROFILE=$(SOURCE_PROFILE) \
		FAB_ROOT=$(abspath $(FAB_ROOT)) verify
	$(MAKE) contract-linked-check
	$(MAKE) test
	$(MAKE) modules
