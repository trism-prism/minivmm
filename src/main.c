#include "dev/dev.h"
#include "exits.h"
#include "loader.h"
#include "stats.h"
#include "vm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	bool stats = false, evfd = false;
	int i = 1;
	for (; i < argc - 1; i++) {
		if (strcmp(argv[i], "--stats") == 0)
			stats = true;
		else if (strcmp(argv[i], "--ioeventfd") == 0)
			evfd = true;
		else
			break;
	}
	if (i != argc - 1 || strncmp(argv[i], "--", 2) == 0) { /* a trailing flag is a missing image path */
		fprintf(stderr, "usage: minivmm [--stats] [--ioeventfd] guest.bin\n");
		return 2;
	}
	if (stats)
		stats_enable();
	signal(SIGPIPE, SIG_IGN); /* a closed stdout must not kill the VMM mid-run */
	atexit(serial_flush); /* die() and any other exit path still emit what the guest printed */
	struct vm v;
	vm_create(&v);
	if (evfd)
		vm_ioeventfd(&v, 0xE9); /* the bench sink, handled in the kernel */
	loader_load(&v, argv[argc - 1]);
	int rc = exits_run(&v);
	serial_flush();
	if (stats)
		stats_print(stderr);
	if (stats && evfd) {
		uint64_t n = 0; /* stays 0 if the guest never wrote (nonblocking read fails with EAGAIN) */
		if (read(v.evfd, &n, sizeof n) < 0)
			n = 0;
		fprintf(stderr, "ioeventfd signals %llu\n", (unsigned long long)n);
	}
	vm_destroy(&v);
	exit(rc);
}
