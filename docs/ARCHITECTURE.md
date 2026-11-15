# Architecture

| File | Role |
|---|---|
| `src/main.c` | CLI: create VM, load image, run, exit with the result |
| `src/vm.c` | `/dev/kvm`, VM, 1 MiB RAM at guest 0 minus the 0xA0000-0xBFFFF hole (two memory slots), vCPU, real-mode reset state (CS=0, RIP/RSP=0x7C00) |
| `src/loader.c` | Read a flat binary into RAM at 0x7C00 |
| `src/exits.c` | `KVM_RUN` loop and dispatcher: port I/O (looped over `count`) and MMIO go to the devices; HLT returns 0; a debug-exit write returns its byte; triple fault, entry failure, unowned port/address and any other exit print a register dump and return 1 |
| `src/dev/dev.h` | Device handler signatures and the `DEV_NONE/OK/EXIT` result |
| `src/dev/serial.c` | Port 0x3F8 OUT: one `write(1)` per byte (unbuffered baseline); 0x3FD IN: 0x60 so polling guests proceed |
| `src/dev/debug_exit.c` | Port 0xF4 OUT (byte): ends the run, byte is the exit status |
| `src/dev/mmio_probe.c` | Guest-physical 0xA0000-0xBFFFF (no RAM): reads zero, writes discarded |
| `src/util.h` | `die()` and the checked `KIOCTL` macro |

Each device is a function taking (port or address, direction, data, size); port I/O tries serial then debug-exit; MMIO goes to the probe and treats `DEV_NONE` from all as an unhandled access.
