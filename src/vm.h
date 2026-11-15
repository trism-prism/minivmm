#ifndef VM_H
#define VM_H

#include <linux/kvm.h>
#include <stddef.h>
#include <stdint.h>

#define RAM_SIZE   (1u << 20)
#define VGA_BASE   0xA0000u	/* start of the unbacked MMIO hole */
#define VGA_END    0xC0000u
#define LOAD_ADDR  0x7C00u

struct vm {
	int kvm, vm, vcpu;
	int evfd;	/* ioeventfd counter, -1 until vm_ioeventfd() */
	uint8_t *ram;
	struct kvm_run *run;
	size_t run_size;
};

void vm_create(struct vm *v);
void vm_destroy(struct vm *v);
void vm_ioeventfd(struct vm *v, uint16_t port);

#endif
