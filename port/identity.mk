# Human-readable identity for ordinary development builds.  The former
# agon-emos-v0.1.0 candidate names the rejected/superseded PORT-008 predecessor
# and must not be silently applied to the current production integration.
# Until the Author approves a new source identity, every ordinary build remains
# visibly unidentified and non-deployable.
EMOS_PROFILE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
EMOS_SOURCE_IDENTITY := UNVERSIONED-DO-NOT-DEPLOY
EMOS_ARTIFACT_STATUS := UNVERSIONED-DO-NOT-DEPLOY
EMOS_BUILD_ID ?= UNVERSIONED-DO-NOT-DEPLOY

# Preserve each value as a C string literal through the outer and nested Make
# command lines used by mos-agondev.
EMOS_IDENTITY_CPPFLAGS := \
	-DEMOS_SOURCE_IDENTITY=\\\"$(EMOS_SOURCE_IDENTITY)\\\" \
	-DEMOS_BUILD_ID=\\\"$(EMOS_BUILD_ID)\\\" \
	-DEMOS_ARTIFACT_STATUS=\\\"$(EMOS_ARTIFACT_STATUS)\\\"
