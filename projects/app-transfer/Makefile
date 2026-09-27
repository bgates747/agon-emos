NAME = apptransfer
AGONDEV_TOOLCHAIN ?= $(shell agondev-config --prefix)
include $(AGONDEV_TOOLCHAIN)/config/makefile.inc
CFLAGS += -Wall -Wextra -Werror -I../../lib/sdapp -I../sdserve/src
OBJS += obj/sdapp.o obj/sdapp_gateway.o obj/sd_engine.o obj/emos_gateway.o
obj/sdapp.o: ../../lib/sdapp/sdapp.c ../../lib/sdapp/sdapp.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@
obj/sdapp_gateway.o: ../../lib/sdapp/gateway.c ../../lib/sdapp/sdapp.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@
obj/sd_engine.o: ../sdserve/src/service.c ../sdserve/src/service.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@
obj/emos_gateway.o: ../sdserve/src/emos_gateway.asm
	@mkdir -p obj
	$(ASM) $(ASMFLAGS) $< -o $@

$(LINKBINARY): obj/sdapp.o obj/sdapp_gateway.o obj/sd_engine.o obj/emos_gateway.o
