#include "exits.h"
#include "stats.h"
#include "util.h"
#include "dev/dev.h"

static void dump_regs(struct vm *v)
{
	struct kvm_regs r;
	struct kvm_sregs s;
	/* Snapshot general registers for the failure report. */
	KIOCTL(v->vcpu, KVM_GET_REGS, &r);
	/* Snapshot segment registers (need CS). */
	KIOCTL(v->vcpu, KVM_GET_SREGS, &s);
	fprintf(stderr, "  rip=%#llx rflags=%#llx cs=%#x:%#llx rsp=%#llx\n",
		r.rip, r.rflags, s.cs.selector, s.cs.base, r.rsp);
	fprintf(stderr, "  rax=%#llx rbx=%#llx rcx=%#llx rdx=%#llx\n",
		r.rax, r.rbx, r.rcx, r.rdx);
}

/* Port I/O, repeated run->io.count times for string ops. Returns -1 to continue, else the exit code. */
static int handle_io(struct vm *v)
{
	struct kvm_run *run = v->run;
	bool w = run->io.direction == KVM_EXIT_IO_OUT;
	uint8_t *p = (uint8_t *)run + run->io.data_offset;
	for (uint32_t i = 0; i < run->io.count; i++, p += run->io.size) {
		enum dev_result r = serial_io(run->io.port, w, p, run->io.size);
		if (r == DEV_NONE)
			r = debug_exit_io(run->io.port, w, p, run->io.size);
		if (r == DEV_EXIT)
			return p[0];
		if (r == DEV_NONE) {
			serial_flush(); /* keep guest output ahead of the diagnostic */
			fprintf(stderr, "minivmm: unhandled I/O port %#x\n", run->io.port);
			dump_regs(v);
			return 1;
		}
	}
	return -1;
}

static int handle_mmio(struct vm *v)
{
	struct kvm_run *run = v->run;
	/* A read's result is picked up by KVM from run->mmio.data on the next KVM_RUN. */
	if (mmio_probe(run->mmio.phys_addr, run->mmio.is_write, run->mmio.data, run->mmio.len) != DEV_NONE)
		return -1;
	serial_flush(); /* keep guest output ahead of the diagnostic */
	fprintf(stderr, "minivmm: unhandled MMIO access at %#llx\n", (unsigned long long)run->mmio.phys_addr);
	dump_regs(v);
	return 1;
}

int exits_run(struct vm *v)
{
	for (;;) {
		/* Enter the guest; returns when it exits to userspace. */
		/* A signal (e.g. Ctrl-Z) interrupts KVM_RUN with EINTR; just re-enter. */
		stats_run_begin();
		while (ioctl(v->vcpu, KVM_RUN, (void *)0) < 0)
			if (errno != EINTR)
				die("KVM_RUN failed: %s", strerror(errno));
		stats_run_end(v->run->exit_reason);
		if (v->run->exit_reason != KVM_EXIT_IO && v->run->exit_reason != KVM_EXIT_MMIO &&
		    v->run->exit_reason != KVM_EXIT_HLT)
			serial_flush(); /* every remaining case prints a diagnostic: guest output first */
		switch (v->run->exit_reason) {
		case KVM_EXIT_IO:
		case KVM_EXIT_MMIO: {
			stats_handler_begin();
			int rc = v->run->exit_reason == KVM_EXIT_IO ? handle_io(v) : handle_mmio(v);
			stats_handler_end();
			if (rc >= 0)
				return rc;
			break;
		}
		case KVM_EXIT_HLT:
			return 0;
		case KVM_EXIT_SHUTDOWN:
			fprintf(stderr, "minivmm: guest triple fault\n");
			dump_regs(v);
			return 1;
		case KVM_EXIT_FAIL_ENTRY:
			fprintf(stderr, "minivmm: VM entry failed, hardware reason %#llx\n",
				(unsigned long long)v->run->fail_entry.hardware_entry_failure_reason);
			dump_regs(v);
			return 1;
		case KVM_EXIT_INTERNAL_ERROR:
			fprintf(stderr, "minivmm: KVM internal error, suberror %u\n", v->run->internal.suberror);
			dump_regs(v);
			return 1;
		default:
			fprintf(stderr, "minivmm: unhandled exit reason %u\n", v->run->exit_reason);
			dump_regs(v);
			return 1;
		}
	}
}
