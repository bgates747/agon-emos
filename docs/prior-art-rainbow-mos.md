# Rainbow MOS prior-art audit (PORT-207)

Credit: Rainbow MOS is copyright Tom Morton and contributors, derived from
Quark MOS by Dean Belfield and Console8 MOS. The audited repository is MIT
licensed. No Rainbow MOS code is imported by this project.

The public source was reviewed and built at commit
`21d13d45f5546f23a057c132f774be4dfbee3a49` (2026-07-31). `LICENSE` has SHA-256
`fae2cacdc47dc5ed8efbd052201834108167942f0b524b8238327e9fbe13fd25`.
With pinned AgonDev `b67ab244...`, `make DEBUG=0` produced an 83,380-byte
binary, SHA-256
`12c7c609a7bd25d32f27a7947101e39a054024817b3da210978f83a0fc3ebe4a`.
These are audit observations, not imported release artifacts.

| Technique | Finding | Decision |
|---|---|---|
| Lineage/version | Separate fork from Console8/Quark through 2.3.3, selectively carrying MOS 3 FatFS and RTC/seek APIs | Unsuitable as an EMOS upstream replacement; useful independent evidence |
| Direct GNU port | Maintains `.S` preprocessed GNU assembly and AgonDev-oriented C directly | Already shared in outcome, not process: this project deliberately retains upstream ZDS-shaped source and deterministic conversion so later Platform MOS merges remain reviewable |
| Assembly conversion | Hand-maintained GNU syntax, CPP feature switches, native `.d24`/sections | Do not import; the strict converter provides source maps, provenance, negative tests, and reproducible regeneration absent from the direct-fork model |
| Startup/linker | Compact GNU linker script, ROM at zero, RAM at `0xbe000`, copied data and cleared BSS, 8 KiB system boundary | Useful comparison. Do not adopt the 8 KiB boundary without Platform MOS/EMOS memory and hardware qualification |
| Compiler flags | `-Oz`, modern AgonDev, and `libagon.a`; DEBUG defaults on | `-Oz` already shared. This project retains stricter freestanding flags, explicit runtime member allow-list, orphan rejection, and linked-image verification |
| FatFS workaround | Extracts `_mount_volume_fsinfo_fragment`, compiles that routine with optimization off to avoid register-allocation failure | Follow-up only if current Platform MOS FatFS reproduces the failure. Current build succeeds at `-Oz`; importing a workaround without reproduction would add divergence |
| Runtime/libc | Bundled formatter and allocator plus broad automatic `libagon.a` selection | Formatter/allocator ideas are already shared; this project keeps its audited nanoprintf subset and explicit 67-member runtime rather than an unconstrained archive link |
| Public ABI | Adds framebuffer, keyboard-event, stdout, and reset-vector APIs while retaining a 2.3.3-centered contract | Do not import into EMOS v1. The APIs are valuable design references but collide with a different compatibility and ownership model |
| Size/performance | Demonstrates a feature-rich 83,380-byte AgonDev image and 8 KiB RAM placement | Adopt as a comparative target, not equivalence evidence; feature sets and memory maps differ |
| Evidence | Repository builds cleanly with the pinned compiler and includes target tests/examples | No candidate/reference emulator parity, frozen artifact audit, or physical qualification was found in the reviewed tree; retain this project's stricter gates |

The principal lesson is that a full direct AgonDev MOS is viable and can be
small. Its maintenance model is intentionally different from this project's
upstream-port pipeline. The only immediate adoption is durable credit and the
independent size/build comparison; the FatFS extraction remains a conditional
follow-up triggered by a reproduced compiler failure.

