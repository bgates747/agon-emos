/*
 * Non-release fixed-backend qualification composition for PORT-008-D002.
 *
 * Its sole substitution is the qualification procedure's assertion that the
 * operator/top-level has already placed the P4 peer in an active parallel
 * epoch.  The explicit EMOS mode request supplies that assertion; EMOS does
 * not detect an external attestation channel.  This file emits no activation,
 * General Poll, response, mode commit, or application-visible command.  Normal
 * and release EMOS profiles do not compile it.
 */

#include "emos_parallel.h"

static BYTE emos_parallel_fixed_peer_ready(void *context) {
	const BYTE *qualificationAssertion = context;
	return qualificationAssertion != 0 && *qualificationAssertion != 0;
}

BYTE emos_parallel_fixed_enter(BYTE qualificationAssertion) {
	return emos_parallel_route_enter(
		emos_parallel_fixed_peer_ready,
		&qualificationAssertion);
}

BYTE emos_parallel_fixed_ready(void) {
	return emos_parallel_route_owned();
}

BYTE emos_parallel_fixed_leave(void) {
	return emos_parallel_route_leave();
}
