#define _GNU_SOURCE /* MAP_ANONYMOUS, O_CLOEXEC */
#include "vm.h"
#include "util.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void vm_create(struct vm *v)
{
	/* MINIVMM_KVM overrides the device path; the tests use it to simulate a missing /dev/kvm. */
	const char *dev = getenv("MINIVMM_KVM");
	if (!dev)
		dev = "/dev/kvm";
	v->kvm = open(dev, O_RDWR | O_CLOEXEC);
	if (v->kvm < 0)
		die("cannot open %s: %s", dev, strerror(errno));
	/* Refuse any KVM API other than the stable version 12. */
	int api = KIOCTL(v->kvm, KVM_GET_API_VERSION, 0);
	if (api != 12)
		die("/dev/kvm API version %d, need 12", api);

	/* New VM; returns the VM fd. */
	v->vm = KIOCTL(v->kvm, KVM_CREATE_VM, 0);

	v->ram = mmap(NULL, RAM_SIZE, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (v->ram == MAP_FAILED)
		die("mmap guest RAM failed: %s", strerror(errno));
	/* Two slots leave 0xA0000-0xBFFFF (the VGA window) unbacked, so guest accesses exit as MMIO. */
	struct kvm_userspace_memory_region mr[2] = {
		{ .slot = 0, .guest_phys_addr = 0, .memory_size = VGA_BASE,
		  .userspace_addr = (uintptr_t)v->ram },
		{ .slot = 1, .guest_phys_addr = VGA_END, .memory_size = RAM_SIZE - VGA_END,
		  .userspace_addr = (uintptr_t)v->ram + VGA_END },
	};
	/* Map host RAM into the guest physical address space around the hole. */
	KIOCTL(v->vm, KVM_SET_USER_MEMORY_REGION, &mr[0]);
	KIOCTL(v->vm, KVM_SET_USER_MEMORY_REGION, &mr[1]);

	/* One vCPU, id 0; returns the vCPU fd. */
	v->vcpu = KIOCTL(v->vm, KVM_CREATE_VCPU, 0);

	/* Size of the shared kvm_run area (asked of the /dev/kvm fd, not the VM). */
	v->run_size = (size_t)KIOCTL(v->kvm, KVM_GET_VCPU_MMAP_SIZE, 0);
	v->run = mmap(NULL, v->run_size, PROT_READ | PROT_WRITE, MAP_SHARED, v->vcpu, 0);
	if (v->run == MAP_FAILED)
		die("mmap kvm_run failed: %s", strerror(errno));

	/* Reset state has CS.base=0xFFFF0000; read sregs so only CS is changed. */
	struct kvm_sregs sr;
	KIOCTL(v->vcpu, KVM_GET_SREGS, &sr);
	sr.cs.selector = 0;
	sr.cs.base = 0;
	/* Write back with real-mode CS at 0. */
	KIOCTL(v->vcpu, KVM_SET_SREGS, &sr);

	struct kvm_regs r = { .rip = LOAD_ADDR, .rflags = 0x2, .rsp = LOAD_ADDR };
	/* Entry point, mandatory rflags bit 1, stack growing down below the image. */
	KIOCTL(v->vcpu, KVM_SET_REGS, &r);
}

void vm_destroy(struct vm *v)
{
	munmap(v->run, v->run_size);
	munmap(v->ram, RAM_SIZE);
	close(v->vcpu);
	close(v->vm);
	close(v->kvm);
}
