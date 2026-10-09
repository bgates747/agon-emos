/* Shared original INTEG-004 clock/deadlines; caller supplies the low-byte clock. */
#ifndef EMOS_PROBE_CLOCK_H
#define EMOS_PROBE_CLOCK_H
/* MOS increments the low clock byte by two per VBlank. At 60 Hz, 600 units
 * allow about five seconds. This is not a millisecond clock. Sampling one
 * byte avoids torn reads; frequent polling accumulates across byte wraps.
 * The separate poll budget terminates even if the VBlank clock stops.
 */
#define TX_CLOCK_LIMIT 120
#define RX_CLOCK_LIMIT 600
#define QUIET_CLOCK_UNITS 24
#define STALLED_POLL_LIMIT 65535UL

typedef struct {
    BYTE last;
    UINT16 elapsed;
    UINT32 stalled;
} ProbeClock;

static void clock_start(ProbeClock *clock) {
    clock->last = emos_uart_probe_clock();
    clock->elapsed = 0;
    clock->stalled = STALLED_POLL_LIMIT;
}

static BYTE clock_step(ProbeClock *clock) {
    BYTE now = emos_uart_probe_clock();
    BYTE delta = (BYTE)(now - clock->last);
    clock->last = now;
    clock->elapsed += delta;
    if (delta) clock->stalled = STALLED_POLL_LIMIT;
    else if (--clock->stalled == 0) return 0;
    return 1;
}

#endif
