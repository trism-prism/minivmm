#include "exits.h"
#include "util.h"

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

int exits_run(struct vm *v)
{
	for (;;) {
		/* Enter the guest; returns when it exits to userspace. */
		/* A signal (e.g. Ctrl-Z) interrupts KVM_RUN with EINTR; just re-enter. */
		while (ioctl(v->vcpu, KVM_RUN, (void *)0) < 0)
			if (errno != EINTR)
				die("KVM_RUN failed: %s", strerror(errno));
		switch (v->run->exit_reason) {
		case KVM_EXIT_HLT:
			return 0;
		case KVM_EXIT_SHUTDOWN:
			fprintf(stderr, "minivmm: guest triple fault\n");
			dump_regs(v);
			return 1;
		default:
			fprintf(stderr, "minivmm: unhandled exit reason %u\n", v->run->exit_reason);
			dump_regs(v);
			return 1;
		}
	}
}
