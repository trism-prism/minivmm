#ifndef STATS_H
#define STATS_H

#include <stdio.h>

/* Exit-cost accounting. Timestamps are taken on every exit, always (see now_ns in stats.c). */
void stats_run_begin(void);			/* just before KVM_RUN */
void stats_run_end(unsigned exit_reason);	/* just after KVM_RUN returns */
void stats_handler_begin(void);		/* around the userspace dispatch */
void stats_handler_end(void);
void stats_print(FILE *f);

#endif
