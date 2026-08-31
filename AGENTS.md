# agon-emos project instructions

When this checkout is inside the Author's larger Agon workspace and
`../agon-dev-env/codex/AGENTS.md` exists, read that canonical guidance first.
An independent clone does not require the sibling checkout.

Read `OWNERSHIP.md` before changing source, build integration, tasks, or
qualification evidence. `TODO.md` is this repository's only authoritative
unfinished-work list.

Maintain EMOS as an upstream-shaped derivative of official MOS. Preserve
official names, source placement, ZDS idioms, APIs, and control flow wherever
practical so later tagged upstream releases remain reviewable. Put EMOS product
behavior in this repository. Put reusable AgonDev port machinery in
`mos-agondev`; never hand-edit generated prepared source or GNU-as output.

Review official documentation before changing MOS APIs, ABIs, sysvars,
executable formats, VDP packets, or hardware contracts. Record deviations,
workarounds, and inherited defects beside affected code and in the active task.
Every emulator-coupled change requires Author validation before commit or push.

Build EMOS only through its repository wrappers or the `mos-agondev`
repository-root targets with an EMOS source profile. Those paths must execute
all profile-owned `FIRMWARE_LINK_CHECKS`, including the linked UART divisor
guard. A direct low-level port build is not an EMOS-qualified build, and a Fab
boot cannot validate physical baud because its VDP link exchanges complete
bytes without an independent receiver clock.
