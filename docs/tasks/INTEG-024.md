# INTEG-024 — Application-owned UART1 async owner binding

Implement only the minimal admitted IRQ/status/CTS retry glue for the selected
provisional application driver. Source/state/buffer/serializer live in app RAM;
existing synchronous net provider remains supported. No vanilla vector takeover.

## Scope and research

Parent cross-component plan: Extender docs/tasks/NET-004/EMOS-ASYNC-R01.md.
Baseline2c79c600 native UART-NM/guarded-RFC25 source, installed rawROM129108B,
1964B free. Reuse UART core823bb91 in application space. Read-only MOSv3.0.2
8336409351ee5314e02801a7b72a4f1bb5282519 and docs f9806bd3 before owner changes.
No new numeric MOS syscall; explicit opt-in provider via existing service ABI.

## Instructions and validation

1. [x] Define/apply atomic attach/submit/service/progress/positive-detach contract;
   validate caller/IFF/spans/mode, keep UART1 RX/error/vector with EMOS, exclude
   every other UART1 writer/route/shared-pin operation while attached.
2. [x] One bounded TX burst after RX, one shared LSR/error observation, GPIO CTS
   refusal disables TX IRQ; existing tick retries and foreground progress checks
   later TEMT. Retain first fault and borrow on refusal. No indefinite ISR wait.
3. [x] Remove app hook atomically before storage reclamation; abnormal exit faults
   owned stream, never claims cancellation/completion. Old net behaviour preserved.
4. [x] Add focused actual-source host tests and linked ownership/IRQ/ROM checks.
   Run repository-root native firmware-check retaining all mandatory validators.
   Finite15min normalPiFIFO, serial compile with owned children joined/released.
5. [x] Report complete resident delta/remaining ROM plus exact private SDK/ROM/map
   build pins and rollback to Extender. Physical trial requires separately frozen
   matched consumer/source/kit/current tuple and normal fresh bench custody.

State: implementation/offline composition/native-link checks pass. No install or
physical pass. The earlier independent consumer scheduler case did not acquire
this driver; a separately frozen provisional consumer trial remains required.

## First implementation candidate

Optional EMOS_UART1_APP selects ext.neta through existing66B gateway; explicit
UART-NM/RFC25 only. OPEN4B supplies an ordinary C app hook and returns existing
7B admission; SUBMIT same wire, PROGRESS4 returns0wireidle/31busy/35fault, RELEASE
refuses live/faulted borrow. Hook events start1/service0/fault2; service sample is
the existing RX drain LSR plus CTS STOP in bit0; demand0idle/1TX/2retry/3tail/4fault.
App core is absent from firmware. Existing tick bounded retry/foreground later
TEMT, one shared sample, RXcap excludes stale data and every writer/PortC guard
retained. Native identity advances to experimentalr05; no deployment/adoption.
Host composition/actual app link and mandatory product link checks remain pending.

First compiler297: actual app/core/EMOS C sanitizer composition, synchronous peer
control and native app link PASS. Identified firmware stopped at our new IER C
compound assignment: AgonDev register constants are numeric, not lvalues. No ROM
was produced/installed. Use one bounded native IN0/OUT0 helper retaining RX bits;
unchanged failed source014d74a and logs remain. Recheck under new normal build.

## Offline checked owner binding — 2026-11-07

Source2c4705310a49c7d97fc606bf47344cc36f68ab2e supplies the identified
experimental build agon-emos-eirene-r05-b2026-11-07-06-35-12Z. Full padded131072B
ROM SHA043a0989b08a7429a2c52aca6a037660f8c19c067376173d17af7c46f1238c0f;
map b87c12848dc5f33571bc60bab904a2b789f75082555ef4729ee4232b5e86a109.
Used130141/free931, delta1033B versus the retained working image. New resident
BSS is hook3B/demand1B/provider-owner1B; app engine/count/source/queue absent.
All selected startup/divisor/pad/RX/console/RFC25/NM/app-hook validators pass
through the mandatory native wrapper. Linked vectors/sysvars remain unchanged.

Actual adapter + unchanged UART core + actual owner net/neta C sanitizer tests
and actual eZ80 app link pass under compiler299; all children joined/released.
First297 numeric-IER failure and logs retained; bounded native helper fixed it.
Subsequent app-only OPEN/refused-cleanup and SUBMIT/refused-cleanup association
regressions pass under301/303, with no owner ROM change. The corrected app probe
adds1189B text/19B BSS, including unchanged569B core/30B emitter and excluding
common CRT/consumer RX chain; whole probe3597/78/445, not MechAgon size/CPU proof.
Default synchronous target object is byte-identical and actual synchronous peer
control passes. Current checked SDK is Extender39d4be6f; exact immutable binding
and artifacts remain in ignored cross-component owner records.

Physical trial, CTS reserve under real traffic, useful foreground work and
consumer whole-program size remain unqualified. Working full ROMa25f and matched
P4d9bcb remain installed and retained as rollback; this new ROM is uninstalled.
Extender owns separate EMOS-only installation/full-readback/rollback and explicit
stopped handoffs under a new normal lease; MechAgon owns its separately frozen
trial. No production selection or generic UART code change is made.

## First selected physical trial — failed and rolled back

The exact retained043a ROM was installed and independently compared at all
131072 bytes for the selected MechAgon consumer trial. That one trial reported
UART1 receiver overrun0x02 after source release; positive later drain/close were
incomplete and application result6 deliberately quarantined. Original SD plus
post-reset64KiB040000 RAM match the consumer executable/map and25first-fault
witnesses. No generic core/emitter defect or measured IRQ blackout is established.

Extender completed supported recovery, preservation beforeLOAD, existing feature
cleanup, exact working fulla25fROM rollback/independentreadback and fresh complete
handback. Actual normal release2026-11-07T07:48:31.562200UTC, all targetactors
stopped; P4/native firmware/startup/listener unchanged. No replay or async
correctness/performance/30Hz qualification. Firmware source2c47053/build identity
and exact bytes are preserved; this is a documentation-only result update.

Owner source review identifies handler-entry STOP versus pre-entry masked time,
foreground gateway locking before RX-first service, callbacks under exclusion
and STOP release before IRQ-return restore as correlation seams. MatchedUART1
1152000/8N1 means16character138.889us emptyFIFO scale, not a safe blackout bound.
Existing UART0 WAIT_CTS underDI remains conditional; preserve the consumer's
reciprocal UART0callback+TEMT/NM guard. No fault clearing/forced close follows.
Full source/physical result and remaining investigation are owned in Extender
docs/tasks/NET-004/EMOS-ASYNC-PHYSICAL-R01-RESULT.md and EMOS-ASYNC-R01.md.

## Selected RX-R01 correction under verified MechAgon goal

Publish Extender EMOS-ASYNC-RX-R01 plan before this change. Isolated ownerworktree
preserves exact failed76source/ROM. Optional async gateway uses one private
scoped STOP hold entered only with originalIFFenabled/ownedUART1; nestedRXdone
cannot reopen receive before its native pairedexit. Fullreceiverhealth includes
pendingfault/stop/demandFAULT; falseIFF/nestedrefusal preserves outerhold and no
EIthenCSTOP race. Old genericlocks/synchronousprofile and appengine unchanged.
Ownerprogress rechecks hook/ownership/keyfault afterRX, because RXfault can
abort/reset demand0 and otherwise fabricateidle/doublewriterrelease. Neither
source correction proves cause/cure76OE; standaloneIRQepilogue/UART0WAIT_CTS and
peerSTOPresponsetiming remain separate. Exactnativefit/regressions/identified
build pending; no installation/benchcase or productionselection from this edit.

FirstRXcompiler319 actualCscope/sanitizer/defaultobject/nativeapp checks passed;
oldactualowner negativecontrol fails as expected. FullnativeROM assembled but
mandatoryPortCinventory refused the two new privateSTOPleaves; no check skipped.
Add exact gatedtwo-PC_DRwriter inventory, with independent native IFF/health/EI
sequence validation; retain319failure. NewlinkedROMfit preliminary130422/free650,
not a checked identifiedartifact. Newnormalcompilertranche required.

## Bounded UART0 derivative selected and integrated

Incorporates UART owner05b8b10/5f568c1 bounded CTS derivative. Exact native
blocked/rearm leaves are retained, with optional saved VBlank hook. EMOS
ownership checks retry pending before IER/TEMT: empty FIFO while deferred is
still BUSY. Accepted new submission clears pending only after admission; final
copy clears before callback. No cancellation/route reset API is added; all
route/mode requests use the existing ownership refusal, and actual reboot
clears BSS. No source can be retired by masking TX interrupt alone.

Full linked validators must check finite no-copy defer/rearm, CTS-before-count,
three correct return frames and live-pending ownership before installation.
The standalone generic cost+52/+1BSS does not predict this combined ROM size.
No physical qualification, measured latency gain or76 cause is claimed.


244030d was checked offline but withheld from deployment: foreground mode
request can be interrupted between pending and IER/TEMT reads. The bounded
ownership helper now captures IFF/DI and preserves BC around the entire locked
snapshot, restoring only caller IFF. This closes an identified EMOS race; no
physical76 attribution. No app-return cancellation is implemented or qualified.

## Task-private576000 paired source control

Prepare a derivative of954e3c54 with only ownedUART1 literal576000/8N1/RTSCTS,
explicit paired declaration and stronger linked owner-settings guard. UART0
1152000 arithmetic/contract remains unchanged. No recorder/core/IRQ algorithm or
ordinary default change. Extender owns the paired P4 derivative. No build during
current86 live watch, install or next physical case selected; preserve8de/d9b
and incominga25f rollback. Full native build/negative controls remain pending.
