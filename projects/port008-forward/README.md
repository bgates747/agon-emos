# PORT-008 predecessor fixture — superseded

Status: **predecessor evidence only**. INTEG-002 superseded the fixed-purpose
r01 GPIO sender, its port008-forward EMOS profile, its precommit General Poll,
and the physical procedure formerly documented here on 2026-09-01. None is a
current build, test, deployment, activation, or qualification authority.

This directory retains one still-useful artifact: the deterministic 106-byte
ordinary-VDU application payload. fixture.json vendors the accepted payload
and browser-frame oracle from agon-extender commit c03656c.
build_fixture.py generates an ADL program that enters only through ordinary
RST.LIL 18h; it contains no GPIO access, EDU envelope, discovery operation,
return parser, or Extender-only API.

Run make port008-fixture at the EMOS repository root to reproduce and verify
that historical application fixture. The ignored files under build/ are
disposable outputs and are not source authority. The checker explicitly proves
only the payload bytes and ordinary entry wrapper. It makes no claim about the
removed sender, EMOS mode transition, P4 readiness, General Poll response,
hardware, or any past physical run.

Current forward-data-plane development uses
port/parallel-fixed-qualification.mk, the common production route and epoch
objects, and the gates recorded in docs/tasks/INTEG-002.md. The old
port/port008-forward.mk filename remains only as a fail-fast tombstone so a
historical command cannot silently create an obsolete firmware image.
