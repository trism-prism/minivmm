#ifndef EXITS_H
#define EXITS_H

#include "vm.h"

/* Run the vCPU until the guest stops. Returns 0 on HLT, 1 on a triple fault or unhandled exit; a failing ioctl exits the process via die(). */
int exits_run(struct vm *v);

#endif
