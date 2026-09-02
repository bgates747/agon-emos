/*
 * Production raw-stream to forward-parallel record engine.
 *
 * The engine is hardware-independent by dependency injection, but this is not
 * a test-only implementation: the eZ80 binding uses these exact functions.
 * No VDU grammar, activation message, response inference, or circuit-specific
 * branch belongs here.
 *
 * Interrupts deliberately remain enabled for READY waits and the complete
 * record hot loop.  The busy flag rejects an interrupting writer without
 * changing the active writer's state: the admitted physical record must never
 * be truncated merely because another caller was rejected.
 */

#include "emos_parallel.h"

static void emos_parallel_latch_fault(
	t_emosParallelEngine *engine, BYTE status) {
	if (engine != 0 && engine->firstFault == EMOS_PARALLEL_OK &&
		status != EMOS_PARALLEL_OK) {
		engine->firstFault = status;
	}
}

static void emos_parallel_idle(t_emosParallelEngine *engine) {
	engine->ops->writeControl(engine->context, 1, 1);
}

static BYTE emos_parallel_wait_ready(
	t_emosParallelEngine *engine,
	BYTE expectedReadyN,
	UINT16 timeoutTicks,
	BYTE timeoutStatus) {
	UINT32 started = engine->ops->readTicks(engine->context);
	UINT32 lastTick = started;
	UINT24 stalledPolls = 0;
	for (;;) {
		UINT32 currentTick;
		BYTE readyN = engine->ops->readReadyN(engine->context) ? 1 : 0;
		if (readyN == expectedReadyN) return EMOS_PARALLEL_OK;
		if (engine->firstFault != EMOS_PARALLEL_OK)
			return engine->firstFault;
		currentTick = engine->ops->readTicks(engine->context);
		if ((UINT32)(currentTick - started) >= (UINT32)timeoutTicks)
			return timeoutStatus;
		if (currentTick != lastTick) {
			lastTick = currentTick;
			stalledPolls = 0;
		} else {
			stalledPolls++;
			if (stalledPolls >= engine->clockStallPollLimit)
				return timeoutStatus;
		}
	}
}

static BYTE emos_parallel_write_record(
	t_emosParallelEngine *engine, const BYTE *bytes, UINT16 length) {
	UINT16 index;
	BYTE status;

	emos_parallel_idle(engine);
	status = emos_parallel_wait_ready(
		engine, 0, engine->admissionTimeoutTicks,
		EMOS_PARALLEL_ADMISSION_TIMEOUT);
	if (status != EMOS_PARALLEL_OK) return status;

	/* Assert VALID_N only while CLOCK is high.  Each payload byte is stable
	 * before its one falling sample edge, and every byte receives a complete
	 * rising recovery phase before the next data write. */
	engine->ops->writeControl(engine->context, 0, 1);
	if (engine->firstFault != EMOS_PARALLEL_OK) return engine->firstFault;
	for (index = 0; index < length; index++) {
		engine->ops->writeData(engine->context, bytes[index]);
		if (engine->firstFault != EMOS_PARALLEL_OK)
			return engine->firstFault;
		engine->ops->writeControl(engine->context, 0, 0);
		if (engine->firstFault != EMOS_PARALLEL_OK)
			return engine->firstFault;
		engine->ops->writeControl(engine->context, 0, 1);
		if (engine->firstFault != EMOS_PARALLEL_OK)
			return engine->firstFault;
	}

	/* Physical record termination contributes no parser-visible byte. */
	emos_parallel_idle(engine);
	status = emos_parallel_wait_ready(
		engine, 1, engine->completionTimeoutTicks,
		EMOS_PARALLEL_COMPLETION_TIMEOUT);
	if (status != EMOS_PARALLEL_OK) return status;

	engine->recordsCompleted++;
	engine->bytesCompleted += length;
	return EMOS_PARALLEL_OK;
}

BYTE emos_parallel_engine_configure(
	t_emosParallelEngine *engine,
	const t_emosParallelWireOps *ops,
	void *context,
	UINT16 maximumRecordBytes,
	UINT16 admissionTimeoutTicks,
	UINT16 completionTimeoutTicks,
	UINT24 clockStallPollLimit) {
	if (engine == 0) return EMOS_PARALLEL_INVALID;
	engine->ops = ops;
	engine->context = context;
	engine->maximumRecordBytes = maximumRecordBytes;
	engine->admissionTimeoutTicks = admissionTimeoutTicks;
	engine->completionTimeoutTicks = completionTimeoutTicks;
	engine->clockStallPollLimit = clockStallPollLimit;
	engine->recordsCompleted = 0;
	engine->bytesCompleted = 0;
	engine->configured = 0;
	engine->epochOpen = 0;
	engine->busy = 0;
	engine->firstFault = EMOS_PARALLEL_CONFIG_ERROR;

	if (ops == 0 || ops->readReadyN == 0 || ops->writeData == 0 ||
		ops->writeControl == 0 || ops->readTicks == 0 ||
		maximumRecordBytes == 0 ||
		maximumRecordBytes > EMOS_PARALLEL_MAX_RECORD_BYTES ||
		admissionTimeoutTicks == 0 || completionTimeoutTicks == 0 ||
		clockStallPollLimit == 0) return EMOS_PARALLEL_CONFIG_ERROR;

	engine->configured = 1;
	engine->firstFault = EMOS_PARALLEL_OK;
	return EMOS_PARALLEL_OK;
}

BYTE emos_parallel_engine_open(t_emosParallelEngine *engine) {
	if (engine == 0 || !engine->configured)
		return EMOS_PARALLEL_CONFIG_ERROR;
	if (engine->busy) return EMOS_PARALLEL_BUSY;
	engine->firstFault = EMOS_PARALLEL_OK;
	engine->epochOpen = 1;
	emos_parallel_idle(engine);
	return EMOS_PARALLEL_OK;
}

BYTE emos_parallel_engine_close(t_emosParallelEngine *engine) {
	if (engine == 0 || !engine->configured)
		return EMOS_PARALLEL_CONFIG_ERROR;
	if (engine->busy) return EMOS_PARALLEL_BUSY;
	emos_parallel_idle(engine);
	engine->epochOpen = 0;
	return EMOS_PARALLEL_OK;
}

BYTE emos_parallel_engine_write(
	t_emosParallelEngine *engine, const BYTE *bytes, UINT16 length) {
	UINT16 remaining = length;
	const BYTE *cursor = bytes;
	BYTE status = EMOS_PARALLEL_OK;

	if (engine == 0 || !engine->configured)
		return EMOS_PARALLEL_CONFIG_ERROR;
	if (!engine->epochOpen) {
		emos_parallel_latch_fault(engine, EMOS_PARALLEL_NOT_OWNED);
		return EMOS_PARALLEL_NOT_OWNED;
	}
	/* Test busy before validating this call's payload.  A malformed nested
	 * caller is still only a rejected contender and cannot poison/truncate the
	 * already-admitted writer. */
	if (engine->busy) return EMOS_PARALLEL_BUSY;
	if (engine->firstFault != EMOS_PARALLEL_OK)
		return engine->firstFault;
	if (length != 0 && bytes == 0) {
		emos_parallel_latch_fault(engine, EMOS_PARALLEL_INVALID);
		return EMOS_PARALLEL_INVALID;
	}
	if (length == 0) return EMOS_PARALLEL_OK;
	/* Busy spans every physical record produced for this one raw stream, so
	 * no interrupting call can interleave bytes at a chunk boundary. */
	engine->busy = 1;
	while (remaining != 0 && status == EMOS_PARALLEL_OK) {
		UINT16 amount = remaining > engine->maximumRecordBytes ?
			engine->maximumRecordBytes : remaining;
		status = emos_parallel_write_record(engine, cursor, amount);
		if (status == EMOS_PARALLEL_OK) {
			cursor += amount;
			remaining -= amount;
		}
	}

	/* This is the common success, timeout, invalidation, and reentry exit. */
	emos_parallel_idle(engine);
	engine->busy = 0;
	if (status != EMOS_PARALLEL_OK)
		emos_parallel_latch_fault(engine, status);
	return status;
}

BYTE emos_parallel_engine_fault(const t_emosParallelEngine *engine) {
	if (engine == 0 || !engine->configured)
		return EMOS_PARALLEL_CONFIG_ERROR;
	return engine->firstFault;
}

BYTE emos_parallel_generation_next(UINT24 current, UINT24 *next) {
	if (next == 0) return EMOS_PARALLEL_INVALID;
	if (current == (UINT24)0xFFFFFF)
		return EMOS_PARALLEL_EPOCH_EXHAUSTED;
	*next = current + 1;
	return EMOS_PARALLEL_OK;
}
