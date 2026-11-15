# Architecture

| File | Role |
|---|---|
| `src/main.c` | CLI: create VM, load image, run, exit with the result |
| `src/vm.c` | `/dev/kvm`, VM, 1 MiB RAM at guest 0, vCPU, real-mode reset state (CS=0, RIP/RSP=0x7C00) |
| `src/loader.c` | Read a flat binary into RAM at 0x7C00 |
| `src/exits.c` | `KVM_RUN` loop: HLT returns 0; SHUTDOWN (triple fault) and every other exit print a register dump and return 1 |
| `src/util.h` | `die()` and the checked `KIOCTL` macro |

Devices (port I/O, MMIO) are not handled yet: those exits report as unhandled.
