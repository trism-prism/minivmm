#include "exits.h"
#include "loader.h"
#include "stats.h"
#include "vm.h"

#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	bool stats = argc == 3 && strcmp(argv[1], "--stats") == 0;
	if (argc != 2 && !stats) {
		fprintf(stderr, "usage: minivmm [--stats] guest.bin\n");
		return 2;
	}
	signal(SIGPIPE, SIG_IGN); /* a closed stdout must not kill the VMM mid-run */
	struct vm v;
	vm_create(&v);
	loader_load(&v, argv[argc - 1]);
	int rc = exits_run(&v);
	if (stats)
		stats_print(stderr);
	vm_destroy(&v);
	exit(rc);
}
