NAME = sdserve
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -Wall -Wextra -Werror
ifeq ($(SDSERVE_IDENTIFIED_BUILD),1)
CFLAGS += -include build/build_identity.h
endif

ifeq ($(SDSERVE_PARALLEL_UPLOAD),1)
CFLAGS += -DSDSERVE_PARALLEL_UPLOAD=1
endif
