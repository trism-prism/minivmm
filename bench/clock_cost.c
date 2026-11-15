#define _GNU_SOURCE
/* Cost of one clock_gettime(CLOCK_MONOTONIC_RAW), the call stats.c makes on every exit. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 5000000
#define RUNS 5

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static int cmp(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;
	return (x > y) - (x < y);
}

int main(void)
{
	double per[RUNS];
	for (int r = 0; r < RUNS; r++) {
		double t0 = now_ns();
		for (long i = 0; i < N; i++)
			now_ns();
		per[r] = (now_ns() - t0) / N;
	}
	qsort(per, RUNS, sizeof per[0], cmp);
	printf("%.1f\n", per[RUNS / 2]);	/* median of RUNS, ns per call (includes loop overhead) */
	return 0;
}
