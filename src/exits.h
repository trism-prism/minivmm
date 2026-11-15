#ifndef EXITS_H
#define EXITS_H

#include "vm.h"

/* Run the vCPU until the guest stops. Returns 0 on HLT, the guest's status byte on a debug-exit write (port 0xF4), 1 on a triple fault, entry failure or unhandled I/O, MMIO or exit; a failing ioctl exits the process via die(). */
int exits_run(struct vm *v);

#endif
