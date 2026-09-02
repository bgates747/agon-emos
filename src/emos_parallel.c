/*
 * eZ80F92 binding and local epoch owner for the EMOS production
 * forward-parallel data plane.
 *
 * This file owns only local mux, direction, idle state, and raw record I/O.
 * The EMOS coordinator owns activation, peer identity, mode commitment, and
 * recovery policy.  No General Poll or return-UART behavior belongs here.
 */

#include <ez80.h>

#include "emos_parallel.h"

#define EMOS_PARALLEL_READY_BIT   ((BYTE)0x10) /* PD4, active low input */
#define EMOS_PARALLEL_CLOCK_BIT   ((BYTE)0x20) /* PD5, sender output */
#define EMOS_PARALLEL_VALID_BIT   ((BYTE)0x80) /* PD7, active low output */
#define EMOS_PARALLEL_CONTROL_BITS ((BYTE)0xB0)

extern volatile BYTE serialFlags;
extern volatile BYTE sysvars[];
extern void emos_parallel_io_write_control(UINT24 controlBits);
extern void emos_parallel_io_configure(
	UINT24 ddrBits, UINT24 alt1Bits, UINT24 alt2Bits);
extern void emos_parallel_io_try_lock(
	volatile BYTE *lock, volatile BYTE *status);
extern void emos_parallel_io_try_begin_entry(
	volatile BYTE *lock,
	volatile BYTE *pinsOwned,
	volatile BYTE *deferredFault,
	volatile BYTE *status);
extern void emos_parallel_io_try_reserve_portc(
	volatile BYTE *lock,
	volatile BYTE *pinsOwned,
	volatile BYTE *status);
extern void emos_parallel_io_read_clock(
	volatile BYTE *clockBytes, BYTE *snapshot);
extern void emos_parallel_io_commit_epoch(
	volatile BYTE *lock,
	volatile BYTE *pinsOwned,
	volatile BYTE *deferredFault,
	volatile UINT24 *generation,
	UINT24 *leaseGeneration,
	UINT24 candidateGeneration,
	volatile BYTE *status);

static t_emosParallelEngine emosParallelEngine;
static volatile BYTE emosParallelInitialized;
static volatile BYTE emosParallelInitializationLock;
static volatile BYTE emosParallelPinsOwned;
static BYTE emosParallelConfigStatus;
static volatile BYTE emosParallelBindingStatus;
static volatile UINT24 emosParallelGeneration;
static volatile BYTE emosParallelWriterLock;
static volatile BYTE emosParallelDeferredFault;
static t_emosParallelEpochLease emosParallelRouteLease;

static BYTE emos_parallel_ez80_read_ready(void *context) {
	(void)context;
	return (PD_DR & EMOS_PARALLEL_READY_BIT) ? 1 : 0;
}

static void emos_parallel_ez80_write_data(void *context, BYTE value) {
	(void)context;
	PC_DR = value;
}

static void emos_parallel_ez80_write_control(
	void *context, BYTE validN, BYTE clockHigh) {
	BYTE value = 0;
	(void)context;
	if (validN) value |= EMOS_PARALLEL_VALID_BIT;
	if (clockHigh) value |= EMOS_PARALLEL_CLOCK_BIT;
	emos_parallel_io_write_control(value);
}

static UINT32 emos_parallel_ez80_read_ticks(void *context) {
	BYTE snapshot[4];
	(void)context;
	/* The stock hardware VBLANK interrupt, not either UART receive stream,
	 * updates this four-byte MOS centisecond clock.  EMOS v1 retains the onboard
	 * VDP and that interrupt as the elapsed-time authority.  The engine's
	 * separate clock-stall poll limit is an uncalibrated liveness fuse only; it
	 * never substitutes iteration count for elapsed time while this clock moves.
	 * The helper snapshots exactly four bytes inside one IFF-preserving critical
	 * section; it never masks interrupts across a READY wait or GPIO edge. */
	emos_parallel_io_read_clock(sysvars, snapshot);
	return (UINT32)snapshot[0] |
		((UINT32)snapshot[1] << 8) |
		((UINT32)snapshot[2] << 16) |
		((UINT32)snapshot[3] << 24);
}

static const t_emosParallelWireOps emosParallelWireOps = {
	emos_parallel_ez80_read_ready,
	emos_parallel_ez80_write_data,
	emos_parallel_ez80_write_control,
	emos_parallel_ez80_read_ticks,
};

static void emos_parallel_release_local_pins(void) {
	/* This is a deterministic production release state, not the prototype's
	 * snapshot/restore policy.  The coordinator must explicitly configure any
	 * later UART epoch; stale caller register state is never resurrected. */
	PC_DDR = 0xFF;
	PC_ALT1 = 0;
	PC_ALT2 = 0;
	PC_DR = 0xFF;
	emos_parallel_io_configure(EMOS_PARALLEL_CONTROL_BITS, 0, 0);
	emos_parallel_io_write_control(
		EMOS_PARALLEL_CLOCK_BIT | EMOS_PARALLEL_VALID_BIT);
}

static void emos_parallel_drive_local_pins(void) {
	/* Idle latches are already loaded.  Port C then becomes the complete data
	 * output and only PD5/PD7 become outputs; READY_N remains an input. */
	PC_DR = 0;
	PC_DDR = 0;
	emos_parallel_io_configure(EMOS_PARALLEL_READY_BIT, 0, 0);
}

void emos_parallel_init(void) {
	BYTE lockStatus = EMOS_PARALLEL_BUSY;
	/* Initialization is monotonic for one boot.  In particular, a caller may
	 * not reset the lease generation and manufacture an ABA-valid stale lease.
	 * The separate initialization lock makes first use interrupt-safe: an
	 * interrupting first caller fails BUSY instead of observing or resetting a
	 * partially initialized binding. */
	if (emosParallelInitialized) return;
	emos_parallel_io_try_lock(&emosParallelInitializationLock, &lockStatus);
	if (lockStatus != EMOS_PARALLEL_OK) return;
	if (emosParallelInitialized) {
		emosParallelInitializationLock = 0;
		return;
	}
	emosParallelConfigStatus = emos_parallel_engine_configure(
		&emosParallelEngine,
		&emosParallelWireOps,
		0,
		EMOS_PARALLEL_MAX_RECORD_BYTES,
		EMOS_PARALLEL_DEFAULT_TIMEOUT_TICKS,
		EMOS_PARALLEL_DEFAULT_TIMEOUT_TICKS,
		EMOS_PARALLEL_CLOCK_STALL_POLL_LIMIT);
	emosParallelPinsOwned = 0;
	emosParallelGeneration = 0;
	emosParallelWriterLock = 0;
	emosParallelDeferredFault = EMOS_PARALLEL_OK;
	emosParallelRouteLease.generation = 0;
	emosParallelBindingStatus = emosParallelConfigStatus;
	/* Publish readiness last.  There are no binding-state writes after this
	 * store, so a first interrupting caller cannot have its live epoch erased
	 * when this initializer resumes. */
	emosParallelInitialized = 1;
	emosParallelInitializationLock = 0;
}

static BYTE emos_parallel_ensure_initialized(void) {
	if (!emosParallelInitialized) emos_parallel_init();
	if (!emosParallelInitialized) return EMOS_PARALLEL_BUSY;
	return emosParallelConfigStatus;
}

static void emos_parallel_note_locked_contention(void) {
	/* An interrupting lifecycle or writer request is retained until a complete
	 * leave/re-enter reset.  The lock owner remains responsible for returning
	 * the electrical interface to a safe record or epoch boundary. */
	emosParallelDeferredFault = EMOS_PARALLEL_BUSY;
	emosParallelBindingStatus = EMOS_PARALLEL_BUSY;
}

BYTE emos_parallel_epoch_enter(
	t_emosParallelPeerReady peerReady,
	void *peerContext,
	t_emosParallelEpochLease *lease) {
	BYTE lockStatus = EMOS_PARALLEL_BUSY;
	BYTE status;
	UINT24 candidateGeneration;
	BYTE engineOpened = 0;
	BYTE pinsPrepared = 0;
	status = emos_parallel_ensure_initialized();
	if (status != EMOS_PARALLEL_OK) return status;
	if (lease == 0) return EMOS_PARALLEL_INVALID;
	/* Acquire only from the fully idle state and clear the preceding epoch's
	 * deferred fault in the same IFF-preserving operation.  There is therefore
	 * no post-lock window in which a new contender can be accidentally erased. */
	emos_parallel_io_try_begin_entry(
		&emosParallelWriterLock,
		&emosParallelPinsOwned,
		&emosParallelDeferredFault,
		&lockStatus);
	if (lockStatus != EMOS_PARALLEL_OK) {
		emosParallelBindingStatus = EMOS_PARALLEL_BUSY;
		return EMOS_PARALLEL_BUSY;
	}
	/* Do not wrap and make a very old lease valid again.  Epoch exhaustion is
	 * fail-closed for the remainder of this boot. */
	status = emos_parallel_generation_next(
		emosParallelGeneration, &candidateGeneration);
	if (status != EMOS_PARALLEL_OK) {
		goto release_lock;
	}
	/* UART1 owns PC0/PC1 whenever serialFlags bit 4 is set.  Activation must
	 * quiesce it before this local epoch owner changes Port C. */
	if (serialFlags & 0x10) {
		status = EMOS_PARALLEL_UART1_ACTIVE;
		goto release_lock;
	}

	emos_parallel_release_local_pins();
	pinsPrepared = 1;
	if (peerReady == 0 || !peerReady(peerContext)) {
		status = emosParallelDeferredFault != EMOS_PARALLEL_OK ?
			emosParallelDeferredFault : EMOS_PARALLEL_PEER_NOT_READY;
		goto release_lock;
	}
	if (emosParallelDeferredFault != EMOS_PARALLEL_OK) {
		status = emosParallelDeferredFault;
		goto release_lock;
	}
	emos_parallel_drive_local_pins();
	status = emos_parallel_engine_open(&emosParallelEngine);
	if (status != EMOS_PARALLEL_OK) {
		goto release_lock;
	}
	engineOpened = 1;

	/* Set the visible success status while the lifecycle lock is still held.
	 * The helper then checks the deferred fault and publishes ownership, both
	 * generation copies, and lock release in one tiny IFF-preserving section.
	 * On a deferred fault it leaves the lock held for deterministic cleanup. */
	emosParallelBindingStatus = EMOS_PARALLEL_OK;
	emos_parallel_io_commit_epoch(
		&emosParallelWriterLock,
		&emosParallelPinsOwned,
		&emosParallelDeferredFault,
		&emosParallelGeneration,
		&lease->generation,
		candidateGeneration,
		&status);
	if (status == EMOS_PARALLEL_OK) return EMOS_PARALLEL_OK;

release_lock:
	if (engineOpened) emos_parallel_engine_close(&emosParallelEngine);
	/* Entry never publishes ownership on this path.  The caller-provided lease
	 * is also untouched, so a rejected redundant enter cannot destroy the live
	 * credential that it aliases. */
	if (pinsPrepared) emos_parallel_release_local_pins();
	emosParallelBindingStatus = status;
	emosParallelWriterLock = 0;
	return status;
}

BYTE emos_parallel_epoch_leave(void) {
	BYTE lockStatus = EMOS_PARALLEL_BUSY;
	BYTE status = emos_parallel_ensure_initialized();
	if (status != EMOS_PARALLEL_OK) return status;
	/* Acquire before inspecting ownership.  During entry, pinsOwned is still
	 * zero but the lifecycle lock is held; a reentrant leave must be observed as
	 * BUSY and force that in-progress entry to abort, not report false success. */
	emos_parallel_io_try_lock(&emosParallelWriterLock, &lockStatus);
	if (lockStatus != EMOS_PARALLEL_OK) {
		emos_parallel_note_locked_contention();
		return EMOS_PARALLEL_BUSY;
	}
	if (!emosParallelPinsOwned) {
		emosParallelBindingStatus = EMOS_PARALLEL_OK;
		emosParallelWriterLock = 0;
		return EMOS_PARALLEL_OK;
	}
	/* Revoke the public lease test before changing electrical state.  The lock
	 * proves that no production writer is inside the record engine. */
	emosParallelPinsOwned = 0;
	status = emos_parallel_engine_close(&emosParallelEngine);
	emos_parallel_release_local_pins();
	emosParallelBindingStatus = status;
	emosParallelWriterLock = 0;
	return status;
}

BYTE emos_parallel_write_stream(
	const t_emosParallelEpochLease *lease,
	const BYTE *bytes,
	UINT16 length) {
	BYTE lockStatus = EMOS_PARALLEL_BUSY;
	BYTE status;
	status = emos_parallel_ensure_initialized();
	if (status != EMOS_PARALLEL_OK) return status;
	emos_parallel_io_try_lock(
		&emosParallelWriterLock, &lockStatus);
	if (lockStatus != EMOS_PARALLEL_OK) {
		emos_parallel_note_locked_contention();
		return emosParallelBindingStatus;
	}
	/* Epoch leave and reopen also use the lock. Validate only after acquisition:
	 * every interrupting writer, even one with malformed/stale arguments, is a
	 * BUSY contender and cannot erase its deferred lifecycle indication. */
	if (!emosParallelPinsOwned || lease == 0 || lease->generation == 0 ||
		lease->generation != emosParallelGeneration) {
		emosParallelBindingStatus = EMOS_PARALLEL_NOT_OWNED;
		/* Publish the result before releasing the lock.  An interrupting
		 * lifecycle/writer may then replace it with a newer sticky fault, but
		 * this rejected caller can never resume and overwrite that result. */
		emosParallelWriterLock = 0;
		return EMOS_PARALLEL_NOT_OWNED;
	}
	if (emosParallelDeferredFault != EMOS_PARALLEL_OK) {
		status = emosParallelDeferredFault;
		emosParallelBindingStatus = status;
		emosParallelWriterLock = 0;
		return status;
	}
	/* UART1 is quiesced for the parallel epoch.  UART0/VBLANK and unrelated
	 * interrupt service remain enabled throughout READY waits and record data.
	 * Only each PD read/modify/write helper briefly masks interrupts and restores
	 * the caller's IFF state.  A nested VDU writer receives BUSY and cannot
	 * interleave with or truncate this writer's admitted raw stream. */
	status = emos_parallel_engine_write(&emosParallelEngine, bytes, length);
	emosParallelBindingStatus = status != EMOS_PARALLEL_OK ?
		status : emosParallelDeferredFault;
	emosParallelWriterLock = 0;
	return status;
}

BYTE emos_parallel_write_byte(
	const t_emosParallelEpochLease *lease, BYTE value) {
	return emos_parallel_write_stream(lease, &value, 1);
}

BYTE emos_parallel_last_fault(void) {
	BYTE engineStatus;
	BYTE status = emos_parallel_ensure_initialized();
	if (status != EMOS_PARALLEL_OK) return status;
	engineStatus = emos_parallel_engine_fault(&emosParallelEngine);
	if (engineStatus != EMOS_PARALLEL_OK) return engineStatus;
	if (emosParallelDeferredFault != EMOS_PARALLEL_OK)
		return emosParallelDeferredFault;
	return emosParallelBindingStatus;
}

BYTE emos_parallel_epoch_owned(void) {
	return emosParallelPinsOwned;
}

BYTE emos_parallel_route_enter(
	t_emosParallelPeerReady peerReady, void *peerContext) {
	t_emosParallelEpochLease candidate;
	BYTE status = emos_parallel_epoch_enter(
		peerReady, peerContext, &candidate);
	/* Publish only after the epoch owner has completed pin, peer, engine, and
	 * generation commitment.  A failed redundant entry cannot erase the route
	 * credential already used by an active semantic dispatcher. */
	if (status == EMOS_PARALLEL_OK) emosParallelRouteLease = candidate;
	return status;
}

BYTE emos_parallel_route_leave(void) {
	BYTE status = emos_parallel_epoch_leave();
	/* A contended leave retains both electrical ownership and its credential;
	 * the mode coordinator must not publish Legacy until a later leave wins. */
	if (status == EMOS_PARALLEL_OK) emosParallelRouteLease.generation = 0;
	return status;
}

BYTE emos_parallel_route_write_stream(const BYTE *bytes, UINT16 length) {
	return emos_parallel_write_stream(&emosParallelRouteLease, bytes, length);
}

BYTE emos_parallel_route_write_byte(BYTE value) {
	return emos_parallel_write_byte(&emosParallelRouteLease, value);
}

BYTE emos_parallel_route_owned(void) {
	return emosParallelRouteLease.generation != 0 && emosParallelPinsOwned;
}

BYTE emos_parallel_uart1_guard_acquire(void) {
	BYTE status = emos_parallel_ensure_initialized();
	if (status != EMOS_PARALLEL_OK) return status;
	/* This reservation shares the lifecycle/writer lock but deliberately does
	 * not touch deferredFault.  Rejecting UART1 during an entry or live write
	 * must not poison the parallel epoch that won the transition. */
	emos_parallel_io_try_reserve_portc(
		&emosParallelWriterLock, &emosParallelPinsOwned, &status);
	return status;
}

void emos_parallel_uart1_guard_release(void) {
	/* Only open_UART1() calls this, exactly once after a successful acquire and
	 * after it has published serialFlags bit 4.  A byte store is indivisible on
	 * the eZ80; an interrupting epoch observes either the held lock or UART1. */
	emosParallelWriterLock = 0;
}
