/* PORT-008 PHASE-TIMING-R01: diagnostic-only callsite probes.
 * All aliases enter one register/IFF-preserving leaf. An exact-bound MOSlet
 * owns the RAM recorder; ordinary profiles erase every call at preprocessing.
 * No transport, GPIO or ordinary packet ownership passes to that recorder.
 */
#ifdef EMOS_PHASE_TIMING
#define PHASE(name) emos_phase_##name()
#define DECLARE_PHASE(name) extern void emos_phase_##name(void)
DECLARE_PHASE(reserve_begin); DECLARE_PHASE(reserve_end);
DECLARE_PHASE(offer_begin); DECLARE_PHASE(coordinate_begin);
DECLARE_PHASE(park_end); DECLARE_PHASE(payload_begin); DECLARE_PHASE(payload_end);
DECLARE_PHASE(unpark_end); DECLARE_PHASE(completion_begin); DECLARE_PHASE(completion_end);
DECLARE_PHASE(service_begin); DECLARE_PHASE(service_end);
DECLARE_PHASE(poll_begin); DECLARE_PHASE(poll_end);
DECLARE_PHASE(publish_begin); DECLARE_PHASE(publish_end);
#else
#define PHASE(name) ((void)0)
#endif
