# minivmm

A minimal KVM virtual machine monitor in C. It runs a tiny flat 16-bit real-mode guest binary loaded at 0x7C00, handles its port I/O and MMIO exits, and exits with the guest's status code. Output is checked against QEMU/KVM, and the cost of a VM exit is measured.


## Results
A port-I/O exit costs about 5.3 µs and an MMIO exit about 6.5 µs, against about 0.1 µs for a plain syscall. Almost all of that is inside `KVM_RUN`, so changes to my own handler barely register. Buffering serial output cut handler time from 226 to 30 ns and wall time by about 3%. Gating timestamps showed no distinguishable change. Handling the port write with `ioeventfd` was the only change that moved the dominant cost: about 21% per write.

## Requirements

- Linux x86-64 with KVM. On Windows: WSL2 with nested virtualization.
- `/dev/kvm` must exist and be usable by your user (membership of the `kvm` group).
- `build-essential`, `nasm`, `make`, `git`; `qemu-system-x86` for the reference comparison.

## Build and test

    make          # build/minivmm (-std=c11 -O2 -Wall -Wextra -Werror)
    make asan     # sanitizer build (address, undefined)
    make guests   # assemble guests/*.asm to build/guests/*.bin
    make test     # golden + QEMU diff + negative tests
    make test-asan # same under the sanitizer build
    make clean

See docs/ for decisions, architecture and running notes.
