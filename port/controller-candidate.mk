# INPUT-001 controller appliance; Author approved v0.1.26 and the bounded
# INTEG-015 VDPlet-admission correction v0.1.27 for bench deployment.
include $(dir $(lastword $(MAKEFILE_LIST)))mos-agondev.mk
# Refuse inherited ordinary identities; keep exploratory builds available.
ifeq ($(EMOS_SOURCE_IDENTITY),UNVERSIONED-DO-NOT-DEPLOY)
ifneq ($(EMOS_BUILD_ID),UNVERSIONED-DO-NOT-DEPLOY)
$(error Unversioned controller source requires an unversioned build)
endif
else
ifeq ($(filter agon-emos-v0.1.26 agon-emos-v0.1.27,$(EMOS_SOURCE_IDENTITY)),)
$(error Controller candidate requires approved agon-emos-v0.1.26 or agon-emos-v0.1.27 identity)
endif
ifeq ($(filter $(EMOS_SOURCE_IDENTITY)-b%Z,$(EMOS_BUILD_ID)),)
$(error Controller candidate requires a matching timestamped build)
endif
endif
ifneq ($(EMOS_ARTIFACT_STATUS),experimental)
$(error Controller candidate requires EMOS_ARTIFACT_STATUS=experimental)
endif
C_SOURCES_EXTRA += src/emos_controller.c
C_OBJECT_RELATIVE_EXTRA += src/emos_controller.o
CPPFLAGS_EXTRA += -DEMOS_CONTROLLER_INPUT=1
