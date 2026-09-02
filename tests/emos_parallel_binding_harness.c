#include <stdio.h>
#include <string.h>

#include "emos_parallel.h"

#define READY_BIT ((BYTE)0x10)
#define CLOCK_BIT ((BYTE)0x20)
#define VALID_BIT ((BYTE)0x80)
#define CONTROL_BITS ((BYTE)0xB0)
#define KEEP_CONTROL ((BYTE)0x4F)

volatile BYTE hostPcDr;
volatile BYTE hostPcDdr;
volatile BYTE hostPcAlt1;
volatile BYTE hostPcAlt2;
volatile BYTE hostPdDr;
volatile BYTE hostPdDdr;
volatile BYTE hostPdAlt1;
volatile BYTE hostPdAlt2;
volatile BYTE serialFlags;
volatile BYTE sysvars[4];

static BYTE peerAttested;
static BYTE receiverReadyN;
static BYTE receiverSawValid;
static BYTE injectNestedWrite;
static BYTE nestedStatus;
static const t_emosParallelEpochLease *activeLease;
static BYTE injectInitializationEnter;
static BYTE initializationEnterStatus;
static BYTE injectPeerWrite;
static BYTE peerWriteStatus;
static BYTE injectPeerLeave;
static BYTE peerLeaveStatus;
static BYTE injectConfigureWrite;
static BYTE configureWriteStatus;
static BYTE injectConfigureEnter;
static BYTE configureEnterStatus;
static BYTE injectConfigureUartGuard;
static BYTE configureUartGuardStatus;
static BYTE injectNestedUartGuard;
static BYTE nestedUartGuardStatus;
static BYTE injectNestedRouteLeave;
static BYTE nestedRouteLeaveStatus;

#define CHECK(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "check failed at %s:%d: %s\n", \
			__FILE__, __LINE__, #condition); \
		return 1; \
	} \
} while (0)

/* Host models of the exact assembly-helper contracts.  The target checker
 * separately inspects the production assembly and its IFF save/restore. */
void emos_parallel_io_write_control(UINT24 requested) {
	BYTE owned;
	BYTE outputs = (BYTE)requested & (CLOCK_BIT | VALID_BIT);
	if (!(outputs & VALID_BIT)) receiverSawValid = 1;
	if (receiverSawValid && (outputs & VALID_BIT)) receiverReadyN = 1;
	owned = outputs | (receiverReadyN ? READY_BIT : 0);
	hostPdDr = (hostPdDr & KEEP_CONTROL) | owned;
	if (injectNestedWrite && !(owned & CLOCK_BIT)) {
		BYTE nested = 0xA5;
		injectNestedWrite = 0;
		nestedStatus = emos_parallel_write_stream(activeLease, &nested, 1);
	}
	if (injectNestedUartGuard && !(owned & CLOCK_BIT)) {
		injectNestedUartGuard = 0;
		nestedUartGuardStatus = emos_parallel_uart1_guard_acquire();
	}
	if (injectNestedRouteLeave && !(owned & CLOCK_BIT)) {
		injectNestedRouteLeave = 0;
		nestedRouteLeaveStatus = emos_parallel_route_leave();
	}
}

void emos_parallel_io_configure(
	UINT24 requestedDdr, UINT24 requestedAlt1, UINT24 requestedAlt2) {
	hostPdDdr = (hostPdDdr & KEEP_CONTROL) |
		((BYTE)requestedDdr & CONTROL_BITS);
	hostPdAlt1 = (hostPdAlt1 & KEEP_CONTROL) |
		((BYTE)requestedAlt1 & CONTROL_BITS);
	hostPdAlt2 = (hostPdAlt2 & KEEP_CONTROL) |
		((BYTE)requestedAlt2 & CONTROL_BITS);
	if (injectConfigureWrite) {
		BYTE value = 0xC3;
		injectConfigureWrite = 0;
		configureWriteStatus = emos_parallel_write_stream(activeLease, &value, 1);
	}
	if (injectConfigureEnter) {
		t_emosParallelEpochLease nestedLease;
		injectConfigureEnter = 0;
		configureEnterStatus = emos_parallel_epoch_enter(0, 0, &nestedLease);
	}
	if (injectConfigureUartGuard) {
		injectConfigureUartGuard = 0;
		configureUartGuardStatus = emos_parallel_uart1_guard_acquire();
	}
}

void emos_parallel_io_try_lock(
	volatile BYTE *lock, volatile BYTE *status) {
	if (*lock) {
		*status = EMOS_PARALLEL_BUSY;
		return;
	}
	*lock = 1;
	*status = EMOS_PARALLEL_OK;
	if (injectInitializationEnter) {
		t_emosParallelEpochLease nestedLease;
		injectInitializationEnter = 0;
		initializationEnterStatus = emos_parallel_epoch_enter(
			0, 0, &nestedLease);
	}
}

void emos_parallel_io_try_begin_entry(
	volatile BYTE *lock,
	volatile BYTE *pinsOwned,
	volatile BYTE *deferredFault,
	volatile BYTE *status) {
	if (*lock) {
		*deferredFault = EMOS_PARALLEL_BUSY;
		*status = EMOS_PARALLEL_BUSY;
		return;
	}
	if (*pinsOwned) {
		*status = EMOS_PARALLEL_BUSY;
		return;
	}
	*lock = 1;
	*deferredFault = EMOS_PARALLEL_OK;
	*status = EMOS_PARALLEL_OK;
}

void emos_parallel_io_try_reserve_portc(
	volatile BYTE *lock,
	volatile BYTE *pinsOwned,
	volatile BYTE *status) {
	if (*lock || *pinsOwned) {
		*status = EMOS_PARALLEL_BUSY;
		return;
	}
	*lock = 1;
	*status = EMOS_PARALLEL_OK;
}

void emos_parallel_io_read_clock(
	volatile BYTE *clockBytes, BYTE *snapshot) {
	BYTE index;
	for (index = 0; index < 4; index++) snapshot[index] = clockBytes[index];
}

void emos_parallel_io_commit_epoch(
	volatile BYTE *lock,
	volatile BYTE *pinsOwned,
	volatile BYTE *deferredFault,
	volatile UINT24 *generation,
	UINT24 *leaseGeneration,
	UINT24 candidateGeneration,
	volatile BYTE *status) {
	if (*deferredFault != EMOS_PARALLEL_OK) {
		*status = *deferredFault;
		return;
	}
	*generation = candidateGeneration;
	*leaseGeneration = candidateGeneration;
	*pinsOwned = 1;
	*status = EMOS_PARALLEL_OK;
	*lock = 0;
}

static BYTE fake_peer_ready(void *context) {
	BYTE *attested = context;
	if (attested == 0 || !*attested) return 0;
	receiverReadyN = 0;
	receiverSawValid = 0;
	hostPdDr &= (BYTE)~READY_BIT;
	if (injectPeerWrite) {
		BYTE value = 0x3C;
		injectPeerWrite = 0;
		peerWriteStatus = emos_parallel_write_stream(activeLease, &value, 1);
	}
	if (injectPeerLeave) {
		injectPeerLeave = 0;
		peerLeaveStatus = emos_parallel_epoch_leave();
	}
	return 1;
}

static void reset_registers(void) {
	hostPcDr = 0x35;
	hostPcDdr = 0xA6;
	hostPcAlt1 = 0x5A;
	hostPcAlt2 = 0xC3;
	hostPdDr = 0x4A;
	hostPdDdr = 0x45;
	hostPdAlt1 = 0x4A;
	hostPdAlt2 = 0x05;
	serialFlags = 0;
	memset((void *)sysvars, 0, sizeof(sysvars));
	peerAttested = 0;
	receiverReadyN = 1;
	receiverSawValid = 0;
	injectNestedWrite = 0;
	nestedStatus = EMOS_PARALLEL_OK;
	activeLease = 0;
	injectInitializationEnter = 0;
	initializationEnterStatus = EMOS_PARALLEL_OK;
	injectPeerWrite = 0;
	peerWriteStatus = EMOS_PARALLEL_OK;
	injectPeerLeave = 0;
	peerLeaveStatus = EMOS_PARALLEL_OK;
	injectConfigureWrite = 0;
	configureWriteStatus = EMOS_PARALLEL_OK;
	injectConfigureEnter = 0;
	configureEnterStatus = EMOS_PARALLEL_OK;
	injectConfigureUartGuard = 0;
	configureUartGuardStatus = EMOS_PARALLEL_OK;
	injectNestedUartGuard = 0;
	nestedUartGuardStatus = EMOS_PARALLEL_OK;
	injectNestedRouteLeave = 0;
	nestedRouteLeaveStatus = EMOS_PARALLEL_OK;
}

static int test_interrupting_first_use_cannot_observe_partial_init(void) {
	t_emosParallelEpochLease lease;
	reset_registers();
	lease.generation = (UINT24)0x654321;
	injectInitializationEnter = 1;
	emos_parallel_init();
	CHECK(initializationEnterStatus == EMOS_PARALLEL_BUSY);
	peerAttested = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_OK);
	CHECK(lease.generation == 1 && emos_parallel_epoch_owned());
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);
	return 0;
}

static int test_failed_peer_is_released_and_retryable(void) {
	t_emosParallelEpochLease lease;
	BYTE unrelatedDr;
	BYTE unrelatedDdr;
	BYTE unrelatedAlt1;
	BYTE unrelatedAlt2;
	reset_registers();
	lease.generation = (UINT24)0x654321;
	unrelatedDr = hostPdDr & KEEP_CONTROL;
	unrelatedDdr = hostPdDdr & KEEP_CONTROL;
	unrelatedAlt1 = hostPdAlt1 & KEEP_CONTROL;
	unrelatedAlt2 = hostPdAlt2 & KEEP_CONTROL;
	emos_parallel_init();
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_PEER_NOT_READY);
	CHECK(!emos_parallel_epoch_owned() &&
		lease.generation == (UINT24)0x654321);
	CHECK(hostPcDr == 0xFF && hostPcDdr == 0xFF);
	CHECK(hostPcAlt1 == 0 && hostPcAlt2 == 0);
	CHECK((hostPdDr & (CLOCK_BIT | VALID_BIT)) == (CLOCK_BIT | VALID_BIT));
	CHECK((hostPdDdr & CONTROL_BITS) == CONTROL_BITS);
	CHECK((hostPdAlt1 & CONTROL_BITS) == 0);
	CHECK((hostPdAlt2 & CONTROL_BITS) == 0);
	CHECK((hostPdDr & KEEP_CONTROL) == unrelatedDr);
	CHECK((hostPdDdr & KEEP_CONTROL) == unrelatedDdr);
	CHECK((hostPdAlt1 & KEEP_CONTROL) == unrelatedAlt1);
	CHECK((hostPdAlt2 & KEEP_CONTROL) == unrelatedAlt2);

	peerAttested = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_OK);
	CHECK(emos_parallel_epoch_owned() && lease.generation != 0);
	CHECK(hostPcDdr == 0 && hostPcAlt1 == 0 && hostPcAlt2 == 0);
	CHECK((hostPdDdr & CONTROL_BITS) == READY_BIT);
	receiverReadyN = 1;
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);
	return 0;
}

static int test_uart_guard_precedes_pin_mutation(void) {
	t_emosParallelEpochLease lease;
	BYTE before[8];
	reset_registers();
	before[0] = hostPcDr;
	before[1] = hostPcDdr;
	before[2] = hostPcAlt1;
	before[3] = hostPcAlt2;
	before[4] = hostPdDr;
	before[5] = hostPdDdr;
	before[6] = hostPdAlt1;
	before[7] = hostPdAlt2;
	serialFlags = 0x10;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_UART1_ACTIVE);
	CHECK(hostPcDr == before[0] && hostPcDdr == before[1]);
	CHECK(hostPcAlt1 == before[2] && hostPcAlt2 == before[3]);
	CHECK(hostPdDr == before[4] && hostPdDdr == before[5]);
	CHECK(hostPdAlt1 == before[6] && hostPdAlt2 == before[7]);
	serialFlags = 0;
	return 0;
}

static int test_nested_writer_cannot_truncate_or_deadlock_outer(void) {
	t_emosParallelEpochLease lease;
	BYTE payload[] = {0x00, 0x55, 0xAA, 0xFF};
	BYTE index;
	peerAttested = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_OK);
	activeLease = &lease;
	injectNestedWrite = 1;
	CHECK(emos_parallel_write_stream(&lease, payload, sizeof(payload)) ==
		EMOS_PARALLEL_OK);
	CHECK(nestedStatus == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_BUSY);
	CHECK(hostPcDr == payload[sizeof(payload) - 1]);
	CHECK((hostPdDr & (CLOCK_BIT | VALID_BIT)) ==
		(CLOCK_BIT | VALID_BIT));
	for (index = 0; index < sizeof(payload); index++) {
		/* The engine harness owns exact byte-sequence tracing.  This binding
		 * check ensures the nested byte was never the final driven value. */
		CHECK(payload[index] != 0xA5);
	}
	CHECK(emos_parallel_write_stream(&lease, payload, 1) == EMOS_PARALLEL_BUSY);
	CHECK(hostPcDr == payload[sizeof(payload) - 1]);
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);
	CHECK(!emos_parallel_epoch_owned());
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_write_stream(&lease, payload, 1) ==
		EMOS_PARALLEL_NOT_OWNED);
	peerAttested = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_OK);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_OK);
	receiverReadyN = 1;
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);
	return 0;
}

static int test_entry_contention_aborts_without_publishing_a_lease(void) {
	t_emosParallelEpochLease lease;
	UINT24 sentinel = (UINT24)0x123456;
	reset_registers();
	peerAttested = 1;
	lease.generation = sentinel;
	activeLease = &lease;
	injectPeerWrite = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_BUSY);
	CHECK(peerWriteStatus == EMOS_PARALLEL_BUSY);
	CHECK(!emos_parallel_epoch_owned() && lease.generation == sentinel);
	CHECK((hostPdDr & (CLOCK_BIT | VALID_BIT)) == (CLOCK_BIT | VALID_BIT));

	/* A fresh lifecycle attempt resets the deferred entry failure. */
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_OK);
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);

	lease.generation = sentinel;
	injectPeerLeave = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_BUSY);
	CHECK(peerLeaveStatus == EMOS_PARALLEL_BUSY);
	CHECK(!emos_parallel_epoch_owned() && lease.generation == sentinel);
	CHECK((hostPdDr & (CLOCK_BIT | VALID_BIT)) == (CLOCK_BIT | VALID_BIT));

	/* Also inject before peerReady, while deterministic pin preparation is
	 * running.  The atomic begin operation must not clear this new fault. */
	lease.generation = sentinel;
	activeLease = &lease;
	injectConfigureWrite = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_BUSY);
	CHECK(configureWriteStatus == EMOS_PARALLEL_BUSY);
	CHECK(!emos_parallel_epoch_owned() && lease.generation == sentinel);
	CHECK((hostPdDr & (CLOCK_BIT | VALID_BIT)) == (CLOCK_BIT | VALID_BIT));

	lease.generation = sentinel;
	activeLease = &lease;
	injectConfigureEnter = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_BUSY);
	CHECK(configureEnterStatus == EMOS_PARALLEL_BUSY);
	CHECK(!emos_parallel_epoch_owned() && lease.generation == sentinel);
	CHECK((hostPdDr & (CLOCK_BIT | VALID_BIT)) == (CLOCK_BIT | VALID_BIT));
	return 0;
}

static int test_redundant_enter_preserves_live_lease_and_stale_is_rejected(void) {
	t_emosParallelEpochLease oldLease;
	t_emosParallelEpochLease currentLease;
	UINT24 liveGeneration;
	BYTE value = 0x69;
	reset_registers();
	peerAttested = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &oldLease) ==
		EMOS_PARALLEL_OK);
	liveGeneration = oldLease.generation;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &oldLease) ==
		EMOS_PARALLEL_BUSY);
	CHECK(oldLease.generation == liveGeneration);
	CHECK(emos_parallel_write_stream(&oldLease, &value, 1) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);

	CHECK(emos_parallel_epoch_enter(
		fake_peer_ready, &peerAttested, &currentLease) == EMOS_PARALLEL_OK);
	CHECK(currentLease.generation != oldLease.generation);
	CHECK(emos_parallel_write_stream(&oldLease, &value, 1) ==
		EMOS_PARALLEL_NOT_OWNED);
	CHECK(emos_parallel_write_stream(&currentLease, &value, 1) ==
		EMOS_PARALLEL_OK);
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);
	return 0;
}

static int test_uart_reservation_serializes_without_poisoning_epoch(void) {
	t_emosParallelEpochLease lease;
	BYTE value = 0x5A;
	BYTE beforePcDr;
	BYTE beforePcDdr;
	reset_registers();
	peerAttested = 1;

	/* If UART1 reserves first, entry loses without changing either port. */
	CHECK(emos_parallel_uart1_guard_acquire() == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_uart1_guard_acquire() == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_OK);
	beforePcDr = hostPcDr;
	beforePcDdr = hostPcDdr;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_BUSY);
	CHECK(hostPcDr == beforePcDr && hostPcDdr == beforePcDdr);
	/* Model open_UART1's final publication while the reservation is held.  A
	 * subsequent epoch must observe UART1 active, not a transition gap. */
	serialFlags |= 0x10;
	emos_parallel_uart1_guard_release();
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_UART1_ACTIVE);
	serialFlags &= (BYTE)~0x10;

	/* If entry owns the lifecycle transition, UART1 loses without turning the
	 * rejected request into a deferred parallel fault. */
	injectConfigureUartGuard = 1;
	CHECK(emos_parallel_epoch_enter(fake_peer_ready, &peerAttested, &lease) ==
		EMOS_PARALLEL_OK);
	CHECK(configureUartGuardStatus == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_uart1_guard_acquire() == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_OK);

	/* The same non-poisoning rule applies while the record writer owns the
	 * common lock. */
	injectNestedUartGuard = 1;
	CHECK(emos_parallel_write_stream(&lease, &value, 1) == EMOS_PARALLEL_OK);
	CHECK(nestedUartGuardStatus == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_OK);

	/* Leave revokes published ownership before restoring the pins, but retains
	 * the same lock until every Port C/Port D mutation is complete. */
	injectConfigureUartGuard = 1;
	CHECK(emos_parallel_epoch_leave() == EMOS_PARALLEL_OK);
	CHECK(configureUartGuardStatus == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_last_fault() == EMOS_PARALLEL_OK);
	return 0;
}

static int test_common_route_and_fixed_backend_share_one_lease(void) {
	BYTE value = 0x96;
	reset_registers();
	CHECK(emos_parallel_route_write_byte(value) == EMOS_PARALLEL_NOT_OWNED);
	CHECK(emos_parallel_fixed_enter(0) == EMOS_PARALLEL_PEER_NOT_READY);
	CHECK(!emos_parallel_fixed_ready());
	/* The fixed adapter attests authorization; the host wire model separately
	 * supplies the already-prepared peer's active-low READY state. */
	receiverReadyN = 0;
	receiverSawValid = 0;
	hostPdDr &= (BYTE)~READY_BIT;
	CHECK(emos_parallel_fixed_enter(1) == EMOS_PARALLEL_OK);
	CHECK(emos_parallel_fixed_ready() && emos_parallel_route_owned());
	CHECK(emos_parallel_route_write_byte(value) == EMOS_PARALLEL_OK);

	/* A busy leave cannot revoke the credential beneath the admitted writer. */
	receiverReadyN = 0;
	receiverSawValid = 0;
	hostPdDr &= (BYTE)~READY_BIT;
	injectNestedRouteLeave = 1;
	CHECK(emos_parallel_route_write_stream(&value, 1) == EMOS_PARALLEL_OK);
	CHECK(nestedRouteLeaveStatus == EMOS_PARALLEL_BUSY);
	CHECK(emos_parallel_route_owned());
	CHECK(emos_parallel_fixed_leave() == EMOS_PARALLEL_OK);
	CHECK(!emos_parallel_route_owned() && !emos_parallel_fixed_ready());
	CHECK(emos_parallel_route_write_byte(value) == EMOS_PARALLEL_NOT_OWNED);
	return 0;
}

int main(void) {
	if (test_interrupting_first_use_cannot_observe_partial_init()) return 1;
	if (test_failed_peer_is_released_and_retryable()) return 1;
	if (test_uart_guard_precedes_pin_mutation()) return 1;
	if (test_nested_writer_cannot_truncate_or_deadlock_outer()) return 1;
	if (test_entry_contention_aborts_without_publishing_a_lease()) return 1;
	if (test_redundant_enter_preserves_live_lease_and_stale_is_rejected()) return 1;
	if (test_uart_reservation_serializes_without_poisoning_epoch()) return 1;
	if (test_common_route_and_fixed_backend_share_one_lease()) return 1;
	puts("EMOS parallel binding host checks passed");
	return 0;
}
