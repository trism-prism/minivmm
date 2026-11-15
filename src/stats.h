#ifndef STATS_H
#define STATS_H

#include <stdio.h>

/* Exit-cost accounting. Off by default: every stats_* function below returns immediately (no clock read, no counting) until stats_enable() is called. */
void stats_enable(void);			/* call once, before the vCPU runs (--stats) */
void stats_run_begin(void);			/* just before KVM_RUN */
void stats_run_end(unsigned exit_reason);	/* just after KVM_RUN returns */
void stats_handler_begin(void);		/* around the userspace dispatch */
void stats_handler_end(void);
void stats_print(FILE *f);

#endif
