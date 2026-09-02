;
; EMOS production forward-parallel Port D atomic helpers.
;
; Each helper preserves the caller's IFF2 state and masks only PD4/PD5/PD7.
; The per-edge critical section is one IN0/mask/merge/OUT0 sequence; READY
; waits, data writes, and record/chunk loops remain interruptible.  This is a
; maintained EMOS unit selected through the product source profile, not an
; upstream GPIO modification and not part of the prototype serial adapter.
;

			INCLUDE	"equs.inc"

			.ASSUME	ADL = 1

			DEFINE .STARTUP, SPACE = ROM
			SEGMENT .STARTUP

			XDEF	_emos_parallel_io_write_control
			XDEF	_emos_parallel_io_configure
			XDEF	_emos_parallel_io_try_lock
			XDEF	_emos_parallel_io_try_begin_entry
			XDEF	_emos_parallel_io_try_reserve_portc
			XDEF	_emos_parallel_io_read_clock
			XDEF	_emos_parallel_io_commit_epoch

EMOS_PARALLEL_CONTROL_BITS	EQU	0B0h
EMOS_PARALLEL_KEEP_CONTROL	EQU	04Fh
EMOS_PARALLEL_READY_BIT		EQU	010h
EMOS_PARALLEL_BUSY_STATUS	EQU	0E2h

; void emos_parallel_io_write_control(UINT24 controlBits)
;
; Replace only PD4/PD5/PD7 in PD_DR. The C caller passes the desired owned
; bits in the low byte of its native-width argument.
;
_emos_parallel_io_write_control:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	BC
			LD	A, (IY+6)
			AND	EMOS_PARALLEL_CONTROL_BITS
			; PD4 is always an input in an owned epoch. Keep its output
			; latch released/high so a later direction change cannot drive
			; READY_N low from stale state.
			OR	EMOS_PARALLEL_READY_BIT
			LD	C, A
			LD	A, I
			PUSH	AF
			DI
			IN0	A, (PD_DR)
			AND	EMOS_PARALLEL_KEEP_CONTROL
			OR	C
			OUT0	(PD_DR), A
			POP	AF
			JP	PO, emos_parallel_control_iff_off
			EI
emos_parallel_control_iff_off:
			POP	BC
			LD	SP, IY
			POP	IY
			RET

; void emos_parallel_io_configure(
;     UINT24 ddrBits, UINT24 alt1Bits, UINT24 alt2Bits)
;
; Atomically replace only PD4/PD5/PD7 in the three direction/function
; registers. A one in DDR means input; zeros in ALT1/ALT2 mean GPIO.
;
_emos_parallel_io_configure:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	BC
			PUSH	DE
			LD	A, (IY+6)
			AND	EMOS_PARALLEL_CONTROL_BITS
			LD	B, A
			LD	A, (IY+9)
			AND	EMOS_PARALLEL_CONTROL_BITS
			LD	C, A
			LD	A, (IY+12)
			AND	EMOS_PARALLEL_CONTROL_BITS
			LD	D, A
			LD	A, I
			PUSH	AF
			DI
			IN0	A, (PD_DDR)
			AND	EMOS_PARALLEL_KEEP_CONTROL
			OR	B
			OUT0	(PD_DDR), A
			IN0	A, (PD_ALT1)
			AND	EMOS_PARALLEL_KEEP_CONTROL
			OR	C
			OUT0	(PD_ALT1), A
			IN0	A, (PD_ALT2)
			AND	EMOS_PARALLEL_KEEP_CONTROL
			OR	D
			OUT0	(PD_ALT2), A
			POP	AF
			JP	PO, emos_parallel_configure_iff_off
			EI
emos_parallel_configure_iff_off:
			POP	DE
			POP	BC
			LD	SP, IY
			POP	IY
			RET

; void emos_parallel_io_try_lock(volatile BYTE *lock,
;                                volatile BYTE *status)
;
; Atomically set a zero byte lock. Store 0 for acquired or E2 for busy through
; the second pointer. This closes the interrupt window between a C-level busy
; test and publication of ownership.
;
_emos_parallel_io_try_lock:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	HL
			PUSH	DE
			LD	HL, (IY+6)
			LD	DE, (IY+9)
			LD	A, I
			PUSH	AF
			DI
			LD	A, (HL)
			OR	A, A
			JR	NZ, emos_parallel_lock_busy
			LD	A, 1
			LD	(HL), A
			XOR	A, A
			JR	emos_parallel_lock_store
emos_parallel_lock_busy:
			LD	A, EMOS_PARALLEL_BUSY_STATUS
emos_parallel_lock_store:
			LD	(DE), A
			POP	AF
			JP	PO, emos_parallel_lock_iff_off
			EI
emos_parallel_lock_iff_off:
			POP	DE
			POP	HL
			LD	SP, IY
			POP	IY
			RET

; void emos_parallel_io_try_begin_entry(
;     volatile BYTE *lock,
;     volatile BYTE *pinsOwned,
;     volatile BYTE *deferredFault,
;     volatile BYTE *status)
;
; Start entry only while both the lifecycle lock and published ownership are
; clear. Acquiring the lock and clearing the preceding epoch's deferred fault
; are indivisible, so a contender after acquisition can never be erased.
;
_emos_parallel_io_try_begin_entry:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	HL
			PUSH	DE
			LD	HL, (IY+6)
			LD	A, I
			PUSH	AF
			DI
			LD	A, (HL)
			OR	A, A
			JR	NZ, emos_parallel_begin_locked
			LD	DE, (IY+9)
			LD	A, (DE)
			OR	A, A
			JR	NZ, emos_parallel_begin_owned
			LD	(HL), 1
			LD	DE, (IY+12)
			XOR	A, A
			LD	(DE), A
			JR	emos_parallel_begin_store
	emos_parallel_begin_locked:
			LD	DE, (IY+12)
			LD	A, EMOS_PARALLEL_BUSY_STATUS
			LD	(DE), A
			JR	emos_parallel_begin_store
emos_parallel_begin_owned:
			LD	A, EMOS_PARALLEL_BUSY_STATUS
emos_parallel_begin_store:
			LD	DE, (IY+15)
			LD	(DE), A
			POP	AF
			JP	PO, emos_parallel_begin_iff_off
			EI
emos_parallel_begin_iff_off:
			POP	DE
			POP	HL
			LD	SP, IY
			POP	IY
			RET

; void emos_parallel_io_try_reserve_portc(
;     volatile BYTE *lock,
;     volatile BYTE *pinsOwned,
;     volatile BYTE *status)
;
; Reserve the common Port C transition lock only while no lifecycle/writer is
; active and no parallel epoch owns the pins.  Unlike entry/writer contention,
; a rejected UART1-open request is not a parallel lifecycle fault and therefore
; must not mutate deferredFault or any published binding result.
;
_emos_parallel_io_try_reserve_portc:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	HL
			PUSH	DE
			LD	HL, (IY+6)
			LD	A, I
			PUSH	AF
			DI
			LD	A, (HL)
			OR	A, A
			JR	NZ, emos_parallel_portc_busy
			LD	DE, (IY+9)
			LD	A, (DE)
			OR	A, A
			JR	NZ, emos_parallel_portc_busy
			LD	(HL), 1
			XOR	A, A
			JR	emos_parallel_portc_store
emos_parallel_portc_busy:
			LD	A, EMOS_PARALLEL_BUSY_STATUS
emos_parallel_portc_store:
			LD	DE, (IY+12)
			LD	(DE), A
			POP	AF
			JP	PO, emos_parallel_portc_iff_off
			EI
emos_parallel_portc_iff_off:
			POP	DE
			POP	HL
			LD	SP, IY
			POP	IY
			RET

; void emos_parallel_io_read_clock(volatile BYTE *clockBytes, BYTE *snapshot)
;
; Snapshot the four-byte MOS VBLANK clock without allowing its ISR to split the
; read. This critical section is independent of every READY wait and record.
;
_emos_parallel_io_read_clock:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	BC
			PUSH	HL
			PUSH	DE
			LD	HL, (IY+6)
			LD	DE, (IY+9)
			LD	A, I
			PUSH	AF
			DI
			LD	B, 4
emos_parallel_clock_copy:
			LD	A, (HL)
			LD	(DE), A
			INC	HL
			INC	DE
			DJNZ	emos_parallel_clock_copy
			POP	AF
			JP	PO, emos_parallel_clock_iff_off
			EI
emos_parallel_clock_iff_off:
			POP	DE
			POP	HL
			POP	BC
			LD	SP, IY
			POP	IY
			RET

; void emos_parallel_io_commit_epoch(
;     volatile BYTE *lock,
;     volatile BYTE *pinsOwned,
;     volatile BYTE *deferredFault,
;     volatile UINT24 *generation,
;     UINT24 *leaseGeneration,
;     UINT24 candidateGeneration,
;     volatile BYTE *status)
;
; Complete entry only if no interrupting lifecycle/writer contender was
; deferred. Success publishes ownership and both copies of the generation,
; stores OK, and releases the lifecycle lock as one IFF-preserving operation.
; Failure stores the sticky fault but deliberately leaves the lock held so the
; C owner can close the engine and restore deterministic pin state first.
;
_emos_parallel_io_commit_epoch:
			PUSH	IY
			LD	IY, 0
			ADD	IY, SP
			PUSH	HL
			PUSH	DE
			LD	A, I
			PUSH	AF
			DI
			LD	HL, (IY+12)
			LD	A, (HL)
			OR	A, A
			JR	NZ, emos_parallel_commit_fault
			LD	DE, (IY+21)
			LD	HL, (IY+15)
			LD	(HL), DE
			LD	HL, (IY+18)
			LD	(HL), DE
			LD	HL, (IY+9)
			LD	(HL), 1
			LD	HL, (IY+24)
			XOR	A, A
			LD	(HL), A
			LD	HL, (IY+6)
			LD	(HL), A
			JR	emos_parallel_commit_restore_iff
emos_parallel_commit_fault:
			LD	HL, (IY+24)
			LD	(HL), A
emos_parallel_commit_restore_iff:
			POP	AF
			JP	PO, emos_parallel_commit_iff_off
			EI
emos_parallel_commit_iff_off:
			POP	DE
			POP	HL
			LD	SP, IY
			POP	IY
			RET

			END
