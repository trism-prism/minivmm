#define _GNU_SOURCE /* CLOCK_MONOTONIC_RAW */
#include "stats.h"

#include <stdint.h>
#include <time.h>

#define MAX_REASON 64	/* KVM exit reasons are small integers; larger ones share the last slot */

static uint64_t count[MAX_REASON];
static uint64_t run_ns, handler_ns;	/* time inside KVM_RUN, time in the dispatcher */
static uint64_t t_first, t_run, t_handler;

/* The only clock read. Gating timestamps behind a flag needs a branch at each stats_* call site in exits.c too, since the begin/end pairs must stay matched. */
static uint64_t now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

void stats_run_begin(void)
{
	t_run = now_ns();
	if (!t_first)
		t_first = t_run;
}

void stats_run_end(unsigned exit_reason)
{
	run_ns += now_ns() - t_run;
	count[exit_reason < MAX_REASON ? exit_reason : MAX_REASON - 1]++;
}

void stats_handler_begin(void) { t_handler = now_ns(); }
void stats_handler_end(void) { handler_ns += now_ns() - t_handler; }

static const char *reason_name(unsigned r, char *buf, size_t n)
{
	switch (r) {
	case 2: return "IO";		/* KVM_EXIT_IO */
	case 5: return "HLT";		/* KVM_EXIT_HLT */
	case 6: return "MMIO";		/* KVM_EXIT_MMIO */
	case 8: return "SHUTDOWN";	/* KVM_EXIT_SHUTDOWN */
	}
	snprintf(buf, n, "%u", r);
	return buf;
}

void stats_print(FILE *f)
{
	uint64_t total = 0, wall = t_first ? now_ns() - t_first : 0;
	char buf[16];
	for (unsigned r = 0; r < MAX_REASON; r++) {
		if (count[r])
			fprintf(f, "exit %s %llu\n", reason_name(r, buf, sizeof buf), (unsigned long long)count[r]);
		total += count[r];
	}
	double d = total ? (double)total : 1;
	fprintf(f, "total_exits %llu\n", (unsigned long long)total);
	fprintf(f, "wall_ns %llu (%.1f/exit)\n", (unsigned long long)wall, (double)wall / d);
	fprintf(f, "kvm_run_ns %llu (%.1f/exit)\n", (unsigned long long)run_ns, (double)run_ns / d);
	fprintf(f, "handler_ns %llu (%.1f/exit)\n", (unsigned long long)handler_ns, (double)handler_ns / d);
}
