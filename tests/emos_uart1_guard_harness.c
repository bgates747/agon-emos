#include <stdio.h>

#include "emos_parallel.h"
#include "uart.h"

volatile BYTE hostPcDr;
volatile BYTE hostPcDdr;
volatile BYTE hostPcAlt1;
volatile BYTE hostPcAlt2;
volatile BYTE hostPdDr;
volatile BYTE hostPdDdr;
volatile BYTE hostPdAlt1;
volatile BYTE hostPdAlt2;
volatile BYTE hostUart0BrgL;
volatile BYTE hostUart0BrgH;
volatile BYTE hostUart0Lctl;
volatile BYTE hostUart0Mctl;
volatile BYTE hostUart0Fctl;
volatile BYTE hostUart0Ier;
volatile BYTE hostUart1BrgL;
volatile BYTE hostUart1BrgH;
volatile BYTE hostUart1Lctl;
volatile BYTE hostUart1Mctl;
volatile BYTE hostUart1Fctl;
volatile BYTE hostUart1Ier;
volatile BYTE serialFlags;

typedef struct {
	BYTE pcDr;
	BYTE pcDdr;
	BYTE pcAlt1;
	BYTE pcAlt2;
	BYTE uart1BrgL;
	BYTE uart1BrgH;
	BYTE uart1Lctl;
	BYTE uart1Mctl;
	BYTE uart1Fctl;
	BYTE uart1Ier;
	BYTE flags;
} t_uart1State;

static BYTE requestedGuardResult;
static BYTE guardHeld;
static BYTE guardContractFailed;
static BYTE acquireCalls;
static BYTE releaseCalls;
static t_uart1State initialState;
static t_uart1State expectedFinalState;

#define CHECK(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "check failed at %s:%d: %s\n", \
			__FILE__, __LINE__, #condition); \
		return 1; \
	} \
} while (0)

static t_uart1State snapshot_uart1(void) {
	t_uart1State state;
	state.pcDr = hostPcDr;
	state.pcDdr = hostPcDdr;
	state.pcAlt1 = hostPcAlt1;
	state.pcAlt2 = hostPcAlt2;
	state.uart1BrgL = hostUart1BrgL;
	state.uart1BrgH = hostUart1BrgH;
	state.uart1Lctl = hostUart1Lctl;
	state.uart1Mctl = hostUart1Mctl;
	state.uart1Fctl = hostUart1Fctl;
	state.uart1Ier = hostUart1Ier;
	state.flags = serialFlags;
	return state;
}

static BYTE state_equal(const t_uart1State *left, const t_uart1State *right) {
	return left->pcDr == right->pcDr &&
		left->pcDdr == right->pcDdr &&
		left->pcAlt1 == right->pcAlt1 &&
		left->pcAlt2 == right->pcAlt2 &&
		left->uart1BrgL == right->uart1BrgL &&
		left->uart1BrgH == right->uart1BrgH &&
		left->uart1Lctl == right->uart1Lctl &&
		left->uart1Mctl == right->uart1Mctl &&
		left->uart1Fctl == right->uart1Fctl &&
		left->uart1Ier == right->uart1Ier &&
		left->flags == right->flags;
}

static void reset_state(void) {
	hostPcDr = 0xA1;
	hostPcDdr = 0x42;
	hostPcAlt1 = 0xFE;
	hostPcAlt2 = 0x40;
	hostPdDr = 0x19;
	hostPdDdr = 0x2A;
	hostPdAlt1 = 0x3B;
	hostPdAlt2 = 0x4C;
	hostUart0BrgL = 0x50;
	hostUart0BrgH = 0x51;
	hostUart0Lctl = 0x52;
	hostUart0Mctl = 0x53;
	hostUart0Fctl = 0x54;
	hostUart0Ier = 0x55;
	hostUart1BrgL = 0x61;
	hostUart1BrgH = 0x62;
	hostUart1Lctl = 0x63;
	hostUart1Mctl = 0x64;
	hostUart1Fctl = 0x65;
	hostUart1Ier = 0x66;
	serialFlags = 0xA5;
	guardHeld = 0;
	guardContractFailed = 0;
	acquireCalls = 0;
	releaseCalls = 0;
	initialState = snapshot_uart1();
}

BYTE emos_parallel_uart1_guard_acquire(void) {
	t_uart1State atAcquire = snapshot_uart1();
	acquireCalls++;
	if (guardHeld || !state_equal(&atAcquire, &initialState))
		guardContractFailed = 1;
	if (requestedGuardResult == EMOS_PARALLEL_OK) guardHeld = 1;
	return requestedGuardResult;
}

void emos_parallel_uart1_guard_release(void) {
	t_uart1State atRelease = snapshot_uart1();
	releaseCalls++;
	if (!guardHeld || !state_equal(&atRelease, &expectedFinalState))
		guardContractFailed = 1;
	guardHeld = 0;
}

static UART uart_settings(void) {
	UART uart;
	uart.baudRate = 1152000;
	uart.dataBits = DATABITS_8;
	uart.stopBits = STOPBITS_1;
	uart.parity = PAR_NOPARITY;
	uart.flowControl = FCTL_HW;
	uart.interrupts = 0x15;
	return uart;
}

static int test_busy_rejection_changes_nothing(void) {
	UART uart = uart_settings();
	t_uart1State after;
	reset_state();
	requestedGuardResult = EMOS_PARALLEL_BUSY;
	expectedFinalState = initialState;
	CHECK(open_UART1(&uart) == UART_ERR_FAILURE);
	after = snapshot_uart1();
	CHECK(state_equal(&after, &initialState));
	CHECK(acquireCalls == 1 && releaseCalls == 0);
	CHECK(!guardHeld && !guardContractFailed);
	return 0;
}

static int test_success_holds_guard_through_final_mutation(void) {
	UART uart = uart_settings();
	t_uart1State after;
	reset_state();
	requestedGuardResult = EMOS_PARALLEL_OK;
	expectedFinalState = initialState;
	expectedFinalState.pcDdr = initialState.pcDdr | 0x0B;
	expectedFinalState.pcAlt1 = initialState.pcAlt1 & (BYTE)~0x0B;
	expectedFinalState.pcAlt2 = (initialState.pcAlt2 | 0x03) & (BYTE)~0x08;
	expectedFinalState.uart1BrgL = 1;
	expectedFinalState.uart1BrgH = 0;
	expectedFinalState.uart1Lctl = 0x03;
	expectedFinalState.uart1Mctl = 0;
	expectedFinalState.uart1Fctl = 7;
	expectedFinalState.uart1Ier = uart.interrupts;
	expectedFinalState.flags = (initialState.flags & 0x0F) | 0x30;
	CHECK(open_UART1(&uart) == UART_ERR_NONE);
	after = snapshot_uart1();
	CHECK(state_equal(&after, &expectedFinalState));
	CHECK(acquireCalls == 1 && releaseCalls == 1);
	CHECK(!guardHeld && !guardContractFailed);
	return 0;
}

int main(void) {
	if (test_busy_rejection_changes_nothing()) return 1;
	if (test_success_holds_guard_through_final_mutation()) return 1;
	puts("EMOS UART1 guard host checks passed");
	return 0;
}
