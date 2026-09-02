#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "emos_parallel.h"

#define TRACE_CAPACITY 262200

typedef struct {
	char kind;
	BYTE a;
	BYTE b;
} TraceEvent;

typedef struct {
	t_emosParallelEngine *engine;
	TraceEvent trace[TRACE_CAPACITY];
	BYTE bytes[65535];
	UINT32 traceCount;
	UINT32 byteCount;
	UINT32 tick;
	UINT32 tickStep;
	UINT24 admissionReads;
	UINT24 completionReads;
	int32_t admissionDelay;
	int32_t completionDelay;
	BYTE readyN;
	BYTE stage;
	BYTE nestedAttempted;
	BYTE nestedStatus;
} FakeWire;

#define CHECK(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "check failed at %s:%d: %s\n", \
			__FILE__, __LINE__, #condition); \
		return 1; \
	} \
} while (0)

static void trace(FakeWire *wire, char kind, BYTE a, BYTE b) {
	if (wire->traceCount < TRACE_CAPACITY) {
		wire->trace[wire->traceCount].kind = kind;
		wire->trace[wire->traceCount].a = a;
		wire->trace[wire->traceCount].b = b;
	}
	wire->traceCount++;
}

static BYTE fake_read_ready(void *context) {
	FakeWire *wire = context;
	if (wire->stage == 0) {
		UINT24 reads = wire->admissionReads++;
		if (wire->admissionDelay >= 0 &&
			reads >= (UINT24)wire->admissionDelay) {
			wire->readyN = 0;
		}
	} else if (wire->stage == 2) {
		UINT24 reads = wire->completionReads++;
		if (wire->completionDelay >= 0 &&
			reads >= (UINT24)wire->completionDelay) {
			wire->readyN = 1;
			wire->stage = 0;
			wire->admissionReads = 0;
			wire->completionReads = 0;
		}
	}
	return wire->readyN;
}

static void fake_write_data(void *context, BYTE value) {
	FakeWire *wire = context;
	trace(wire, 'D', value, 0);
	if (wire->byteCount < sizeof(wire->bytes))
		wire->bytes[wire->byteCount] = value;
	wire->byteCount++;
	if (!wire->nestedAttempted) {
		BYTE nested = 0xA5;
		wire->nestedAttempted = 1;
		if (wire->engine != 0)
			wire->nestedStatus = emos_parallel_engine_write(
				wire->engine, &nested, 1);
	}
}

static void fake_write_control(void *context, BYTE validN, BYTE clockHigh) {
	FakeWire *wire = context;
	trace(wire, 'C', validN, clockHigh);
	if (!validN) wire->stage = 1;
	else if (wire->stage == 1) wire->stage = 2;
}

static UINT32 fake_read_ticks(void *context) {
	FakeWire *wire = context;
	UINT32 value = wire->tick;
	wire->tick += wire->tickStep;
	return value;
}

static const t_emosParallelWireOps fakeOps = {
	fake_read_ready,
	fake_write_data,
	fake_write_control,
	fake_read_ticks,
};

static void fake_init(FakeWire *wire) {
	memset(wire, 0, sizeof(*wire));
	wire->readyN = 1;
	wire->admissionDelay = 0;
	wire->completionDelay = 0;
	wire->tickStep = 1;
	wire->nestedAttempted = 1;
	wire->nestedStatus = EMOS_PARALLEL_OK;
}

static BYTE engine_init(
	t_emosParallelEngine *engine, FakeWire *wire, UINT16 recordLimit,
	UINT16 timeout, UINT24 clockStallPollLimit) {
	BYTE status;
	wire->engine = engine;
	status = emos_parallel_engine_configure(
		engine, &fakeOps, wire, recordLimit, timeout, timeout,
		clockStallPollLimit);
	if (status == EMOS_PARALLEL_OK)
		status = emos_parallel_engine_open(engine);
	wire->traceCount = 0;
	return status;
}

static int test_exact_cadence(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE payload[] = {0x00, 0xFF};
	static const TraceEvent expected[] = {
		{'C', 1, 1}, {'C', 0, 1},
		{'D', 0x00, 0}, {'C', 0, 0}, {'C', 0, 1},
		{'D', 0xFF, 0}, {'C', 0, 0}, {'C', 0, 1},
		{'C', 1, 1}, {'C', 1, 1},
	};
	fake_init(&wire);
	CHECK(engine_init(&engine, &wire, 4, 20, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, payload, sizeof(payload)) ==
		EMOS_PARALLEL_OK);
	CHECK(wire.traceCount == sizeof(expected) / sizeof(expected[0]));
	CHECK(memcmp(wire.trace, expected, sizeof(expected)) == 0);
	CHECK(engine.recordsCompleted == 1);
	CHECK(engine.bytesCompleted == 2);
	CHECK(engine.busy == 0 && engine.firstFault == EMOS_PARALLEL_OK);
	return 0;
}

static int test_chunking_and_flattening(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE payload[10];
	BYTE index;
	fake_init(&wire);
	for (index = 0; index < sizeof(payload); index++) payload[index] = index;
	CHECK(engine_init(&engine, &wire, 4, 20, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, payload, sizeof(payload)) ==
		EMOS_PARALLEL_OK);
	CHECK(engine.recordsCompleted == 3);
	CHECK(engine.bytesCompleted == sizeof(payload));
	CHECK(wire.byteCount == sizeof(payload));
	CHECK(memcmp(wire.bytes, payload, sizeof(payload)) == 0);
	return 0;
}

static int test_record_limit_boundaries(void) {
	static BYTE payload[4097];
	static const UINT16 lengths[] = {0, 1, 4095, 4096, 4097};
	static const UINT32 records[] = {0, 1, 1, 1, 2};
	UINT32 caseIndex;
	for (caseIndex = 0;
		caseIndex < sizeof(lengths) / sizeof(lengths[0]); caseIndex++) {
		t_emosParallelEngine engine;
		FakeWire wire;
		UINT16 index;
		fake_init(&wire);
		for (index = 0; index < lengths[caseIndex]; index++)
			payload[index] = (BYTE)index;
		CHECK(engine_init(&engine, &wire, 4096, 20, 100) ==
			EMOS_PARALLEL_OK);
		CHECK(emos_parallel_engine_write(
			&engine, payload, lengths[caseIndex]) == EMOS_PARALLEL_OK);
		CHECK(engine.recordsCompleted == records[caseIndex]);
		CHECK(engine.bytesCompleted == lengths[caseIndex]);
		CHECK(wire.byteCount == lengths[caseIndex]);
		CHECK(memcmp(wire.bytes, payload, lengths[caseIndex]) == 0);
	}
	return 0;
}

static int test_full_uint16_stream(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	static BYTE payload[65535];
	UINT32 index;
	fake_init(&wire);
	for (index = 0; index < sizeof(payload); index++)
		payload[index] = (BYTE)index;
	CHECK(engine_init(&engine, &wire, 4096, 20, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, payload, (UINT16)sizeof(payload)) ==
		EMOS_PARALLEL_OK);
	CHECK(engine.recordsCompleted == 16);
	CHECK(engine.bytesCompleted == sizeof(payload));
	CHECK(wire.byteCount == sizeof(payload));
	CHECK(memcmp(wire.bytes, payload, sizeof(payload)) == 0);
	return 0;
}

static int test_admission_timeout_cleanup(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE value = 7;
	fake_init(&wire);
	wire.admissionDelay = -1;
	CHECK(engine_init(&engine, &wire, 4, 3, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) ==
		EMOS_PARALLEL_ADMISSION_TIMEOUT);
	CHECK(engine.busy == 0);
	CHECK(engine.firstFault == EMOS_PARALLEL_ADMISSION_TIMEOUT);
	CHECK(wire.byteCount == 0);
	CHECK(wire.trace[wire.traceCount - 1].kind == 'C');
	CHECK(wire.trace[wire.traceCount - 1].a == 1);
	CHECK(wire.trace[wire.traceCount - 1].b == 1);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) ==
		EMOS_PARALLEL_ADMISSION_TIMEOUT);
	return 0;
}

static int test_completion_timeout_and_retry(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE value = 9;
	fake_init(&wire);
	wire.completionDelay = -1;
	CHECK(engine_init(&engine, &wire, 4, 3, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) ==
		EMOS_PARALLEL_COMPLETION_TIMEOUT);
	CHECK(engine.busy == 0);
	CHECK(engine.firstFault == EMOS_PARALLEL_COMPLETION_TIMEOUT);
	emos_parallel_engine_close(&engine);
	wire.completionDelay = 0;
	wire.readyN = 1;
	wire.stage = 0;
	CHECK(emos_parallel_engine_open(&engine) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) == EMOS_PARALLEL_OK);
	return 0;
}

static int test_finite_poll_ceiling_with_stalled_ticks(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE value = 1;
	fake_init(&wire);
	wire.admissionDelay = -1;
	wire.tickStep = 0;
	CHECK(engine_init(&engine, &wire, 4, 20, 5) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) ==
		EMOS_PARALLEL_ADMISSION_TIMEOUT);
	CHECK(wire.admissionReads == 5);
	return 0;
}

static int test_tick_wrap(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE value = 1;
	fake_init(&wire);
	wire.admissionDelay = -1;
	wire.tick = UINT32_MAX - 1;
	CHECK(engine_init(&engine, &wire, 4, 3, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) ==
		EMOS_PARALLEL_ADMISSION_TIMEOUT);
	return 0;
}

static int test_reentry_rejects_nested_and_finishes_outer_writer(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE payload[] = {1, 2};
	fake_init(&wire);
	wire.nestedAttempted = 0;
	CHECK(engine_init(&engine, &wire, 4, 20, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, payload, sizeof(payload)) ==
		EMOS_PARALLEL_OK);
	CHECK(wire.nestedStatus == EMOS_PARALLEL_BUSY);
	CHECK(wire.byteCount == sizeof(payload));
	CHECK(memcmp(wire.bytes, payload, sizeof(payload)) == 0);
	CHECK(engine.firstFault == EMOS_PARALLEL_OK && engine.busy == 0);
	CHECK(wire.trace[wire.traceCount - 1].kind == 'C');
	CHECK(wire.trace[wire.traceCount - 1].a == 1);
	CHECK(wire.trace[wire.traceCount - 1].b == 1);
	return 0;
}

static int test_invalid_and_epoch_guards(void) {
	t_emosParallelEngine engine;
	FakeWire wire;
	BYTE value = 1;
	fake_init(&wire);
	CHECK(emos_parallel_engine_configure(
		&engine, &fakeOps, &wire, 0, 1, 1, 1) ==
		EMOS_PARALLEL_CONFIG_ERROR);
	CHECK(emos_parallel_engine_configure(
		&engine, &fakeOps, &wire, 4097, 1, 1, 1) ==
		EMOS_PARALLEL_CONFIG_ERROR);
	CHECK(emos_parallel_engine_configure(
		&engine, &fakeOps, &wire, 4, 3, 3, 100) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, &value, 1) ==
		EMOS_PARALLEL_NOT_OWNED);
	CHECK(emos_parallel_engine_open(&engine) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, 0, 0) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_engine_write(&engine, 0, 1) == EMOS_PARALLEL_INVALID);
	return 0;
}

static int test_generation_fails_closed_instead_of_wrapping(void) {
	UINT24 next = 0;
	CHECK(emos_parallel_generation_next((UINT24)0xFFFFFE, &next) ==
		EMOS_PARALLEL_OK);
	CHECK(next == (UINT24)0xFFFFFF);
	next = (UINT24)0x123456;
	CHECK(emos_parallel_generation_next((UINT24)0xFFFFFF, &next) ==
		EMOS_PARALLEL_EPOCH_EXHAUSTED);
	CHECK(next == (UINT24)0x123456);
	CHECK(emos_parallel_generation_next(0, 0) == EMOS_PARALLEL_INVALID);
	return 0;
}

int main(void) {
	if (test_exact_cadence()) return 1;
	if (test_chunking_and_flattening()) return 1;
	if (test_record_limit_boundaries()) return 1;
	if (test_full_uint16_stream()) return 1;
	if (test_admission_timeout_cleanup()) return 1;
	if (test_completion_timeout_and_retry()) return 1;
	if (test_finite_poll_ceiling_with_stalled_ticks()) return 1;
	if (test_tick_wrap()) return 1;
	if (test_reentry_rejects_nested_and_finishes_outer_writer()) return 1;
	if (test_invalid_and_epoch_guards()) return 1;
	if (test_generation_fails_closed_instead_of_wrapping()) return 1;
	puts("EMOS parallel engine host checks passed");
	return 0;
}
