/*
 * EMOS production forward-parallel data plane.
 *
 * Physical records are deliberately below the raw VDU byte stream.  This
 * interface owns neither activation nor formal-mode commitment.  The EMOS
 * coordinator may open an epoch only after its caller has established peer
 * readiness, and ordinary VDU routing remains coordinator-owned.
 */

#ifndef EMOS_PARALLEL_H
#define EMOS_PARALLEL_H

#include <defines.h>

#define EMOS_PARALLEL_MAX_RECORD_BYTES       ((UINT16)4096)
#define EMOS_PARALLEL_DEFAULT_TIMEOUT_TICKS  ((UINT16)100)
#define EMOS_PARALLEL_CLOCK_STALL_POLL_LIMIT ((UINT24)0xFFFFFF)

#define EMOS_PARALLEL_OK                     ((BYTE)0x00)
#define EMOS_PARALLEL_INVALID                ((BYTE)0xE0)
#define EMOS_PARALLEL_NOT_OWNED              ((BYTE)0xE1)
#define EMOS_PARALLEL_BUSY                   ((BYTE)0xE2)
#define EMOS_PARALLEL_ADMISSION_TIMEOUT      ((BYTE)0xE3)
#define EMOS_PARALLEL_COMPLETION_TIMEOUT     ((BYTE)0xE4)
#define EMOS_PARALLEL_PEER_NOT_READY         ((BYTE)0xE5)
#define EMOS_PARALLEL_UART1_ACTIVE           ((BYTE)0xE6)
#define EMOS_PARALLEL_CONFIG_ERROR           ((BYTE)0xE7)
#define EMOS_PARALLEL_EPOCH_EXHAUSTED        ((BYTE)0xE8)

typedef BYTE (*t_emosParallelReadReadyN)(void *context);
typedef void (*t_emosParallelWriteData)(void *context, BYTE value);
typedef void (*t_emosParallelWriteControl)(
	void *context, BYTE validN, BYTE clockHigh);
typedef UINT32 (*t_emosParallelReadTicks)(void *context);

typedef struct {
	t_emosParallelReadReadyN readReadyN;
	t_emosParallelWriteData writeData;
	t_emosParallelWriteControl writeControl;
	t_emosParallelReadTicks readTicks;
} t_emosParallelWireOps;

/* The structure is exposed so deterministic host tests can execute the exact
 * production engine with injected wire/time operations.  Product callers use
 * only the binding functions declared below. */
typedef struct {
	const t_emosParallelWireOps *ops;
	void *context;
	UINT16 maximumRecordBytes;
	UINT16 admissionTimeoutTicks;
	UINT16 completionTimeoutTicks;
	UINT24 clockStallPollLimit;
	UINT32 recordsCompleted;
	UINT32 bytesCompleted;
	BYTE configured;
	BYTE epochOpen;
	volatile BYTE busy;
	volatile BYTE firstFault;
} t_emosParallelEngine;

BYTE emos_parallel_engine_configure(
	t_emosParallelEngine *engine,
	const t_emosParallelWireOps *ops,
	void *context,
	UINT16 maximumRecordBytes,
	UINT16 admissionTimeoutTicks,
	UINT16 completionTimeoutTicks,
	UINT24 clockStallPollLimit);
BYTE emos_parallel_engine_open(t_emosParallelEngine *engine);
BYTE emos_parallel_engine_close(t_emosParallelEngine *engine);
BYTE emos_parallel_engine_write(
	t_emosParallelEngine *engine, const BYTE *bytes, UINT16 length);
BYTE emos_parallel_engine_fault(const t_emosParallelEngine *engine);
BYTE emos_parallel_generation_next(UINT24 current, UINT24 *next);

/* A lease is private coordinator state, not an application API or transport
 * bypass.  Its generation rejects use after close/reopen. */
typedef struct {
	UINT24 generation;
} t_emosParallelEpochLease;

typedef BYTE (*t_emosParallelPeerReady)(void *context);

void emos_parallel_init(void);
BYTE emos_parallel_epoch_enter(
	t_emosParallelPeerReady peerReady,
	void *peerContext,
	t_emosParallelEpochLease *lease);
BYTE emos_parallel_epoch_leave(void);
BYTE emos_parallel_write_stream(
	const t_emosParallelEpochLease *lease,
	const BYTE *bytes,
	UINT16 length);
BYTE emos_parallel_write_byte(
	const t_emosParallelEpochLease *lease, BYTE value);
BYTE emos_parallel_last_fault(void);
BYTE emos_parallel_epoch_owned(void);

/* Core-private semantic-route bridge.  The bridge, not a qualification
 * backend, retains the live lease used by the assembly VDU dispatcher.  These
 * symbols are not registered in the MOS API or C function table. */
BYTE emos_parallel_route_enter(
	t_emosParallelPeerReady peerReady, void *peerContext);
BYTE emos_parallel_route_leave(void);
BYTE emos_parallel_route_write_stream(const BYTE *bytes, UINT16 length);
BYTE emos_parallel_route_write_byte(BYTE value);
BYTE emos_parallel_route_owned(void);

/* Internal UART1/parallel Port C transition gate.  open_UART1() maps a busy
 * result to its existing generic UART failure value before changing state. */
BYTE emos_parallel_uart1_guard_acquire(void);
void emos_parallel_uart1_guard_release(void);

/* Non-release fixed-composition adapter.  It supplies only the externally
 * prepared peer attestation and lifecycle calls; ordinary bytes still enter
 * through the semantic route above. */
BYTE emos_parallel_fixed_enter(BYTE qualificationAssertion);
BYTE emos_parallel_fixed_ready(void);
BYTE emos_parallel_fixed_leave(void);

#endif /* EMOS_PARALLEL_H */
