# Author-approved v0.1.8 resident keyboard increment; draft emulator review.
# Existing candidate/rollback images retain their original identities.
EMOS_PROFILE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
EMOS_SOURCE_IDENTITY := agon-emos-v0.1.8
EMOS_ARTIFACT_STATUS := draft
# Expand once and export so nested Make invocations retain the same build ID.
# A reviewed build wrapper may supply an already recorded UTC identity.
ifndef EMOS_BUILD_ID
EMOS_BUILD_ID := $(EMOS_SOURCE_IDENTITY)-b$(shell date -u +%Y-%m-%d-%H-%M-%SZ)
endif
export EMOS_BUILD_ID

# The qualification procedure/composition is independently revisioned. Only
# the fixed profile consumes this value; ordinary EMOS builds must not carry
# it. The Author must approve the revision before a candidate build replaces
# this marker.
EMOS_QUALIFICATION_COMPOSITION_IDENTITY ?= \
	UNVERSIONED-PORT008-FORWARD-QUALIFICATION-DO-NOT-DEPLOY

# Preserve each value as a C string literal through the outer and nested Make
# command lines used by mos-agondev.
EMOS_IDENTITY_CPPFLAGS := \
	-DEMOS_SOURCE_IDENTITY=\\\"$(EMOS_SOURCE_IDENTITY)\\\" \
	-DEMOS_BUILD_ID=\\\"$(EMOS_BUILD_ID)\\\" \
	-DEMOS_ARTIFACT_STATUS=\\\"$(EMOS_ARTIFACT_STATUS)\\\"
