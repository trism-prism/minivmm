/* Cost of one kernel crossing, for scale: raw getppid syscall (not the glibc wrapper, which could cache). */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

#define RUNS 5

static int cmp(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;
	return (x > y) - (x < y);
}

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(int argc, char **argv)
{
	long n = argc > 1 ? atol(argv[1]) : 1000000;
	if (n <= 0)
		return 2;
	double per[RUNS];
	for (int r = 0; r < RUNS; r++) {
		double t0 = now_ns();
		for (long i = 0; i < n; i++)
			syscall(SYS_getppid);
		per[r] = (now_ns() - t0) / (double)n;
	}
	qsort(per, RUNS, sizeof per[0], cmp);
	printf("%.1f\n", per[RUNS / 2]);	/* median of RUNS, ns per call */
	return 0;
}
