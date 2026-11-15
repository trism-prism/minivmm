#include "exits.h"
#include "loader.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: minivmm guest.bin\n");
		return 2;
	}
	struct vm v;
	vm_create(&v);
	loader_load(&v, argv[1]);
	int rc = exits_run(&v);
	vm_destroy(&v);
	exit(rc);
}
