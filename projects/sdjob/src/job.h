#ifndef EMOS_SD_JOB_H
#define EMOS_SD_JOB_H
int sdjob_run(void);
/* Target reads Escape; host tests substitute only this platform observation. */
int sdjob_cancelled(void);
#endif
