# minivmm

A minimal KVM virtual machine monitor in C. It runs a tiny flat 16-bit real-mode guest binary loaded at 0x7C00, handles its port I/O and MMIO exits, and exits with the guest's status code. Output is checked against QEMU/KVM, and the cost of a VM exit is measured.

Status: scaffold only. No source code yet.

## Requirements

- Linux x86-64 with KVM. On Windows: WSL2 with nested virtualization.
- `/dev/kvm` must exist and be usable by your user (membership of the `kvm` group).
- `build-essential`, `nasm`, `make`, `git`; `qemu-system-x86` for the reference comparison.

## Build and test

    make          # build/minivmm (-std=c11 -O2 -Wall -Wextra -Werror)
    make asan     # sanitizer build (address, undefined)
    make guests   # assemble guests/*.asm to build/guests/*.bin
    make test     # run tests/run.sh
    make clean

See docs/ for decisions, architecture and running notes.
