# Human-readable EMOS product identity. The source identity and lifecycle state
# are committed; the UTC build ID is supplied by the controlled build. A build
# that omits it remains visibly non-deployable.
EMOS_SOURCE_IDENTITY := agon-emos-v0.1.0
EMOS_ARTIFACT_STATUS := candidate
EMOS_BUILD_ID ?= UNVERSIONED-DO-NOT-DEPLOY

# Preserve each value as a C string literal through the outer and nested Make
# command lines used by mos-agondev.
EMOS_IDENTITY_CPPFLAGS := \
	-DEMOS_SOURCE_IDENTITY=\\\"$(EMOS_SOURCE_IDENTITY)\\\" \
	-DEMOS_BUILD_ID=\\\"$(EMOS_BUILD_ID)\\\" \
	-DEMOS_ARTIFACT_STATUS=\\\"$(EMOS_ARTIFACT_STATUS)\\\"
