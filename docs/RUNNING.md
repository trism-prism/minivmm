# Running

## Install and build

    sudo apt install build-essential nasm make git qemu-system-x86
    make            # build/minivmm
    make guests     # build/guests/*.bin (make test does this too)

QEMU is optional; without it the golden files are the only reference.

## Run

    build/minivmm build/guests/hello.bin

The exit status is the guest's status byte (port 0xF4), or nonzero with a message on stderr if the guest faults or the image is bad.

## Test

    make test         # run.sh (golden + QEMU) and negative.sh against build/minivmm
    make test-asan    # the same against build/minivmm-asan (address and undefined sanitizers)

Each check prints one PASS or FAIL line; the scripts exit nonzero if any failed. Run one script alone with `BIN=build/minivmm tests/run.sh`.

QEMU comparison: for hello, arith, memtest and exit_code, `tests/run.sh` also runs the same image under `qemu-system-x86_64 -enable-kvm` and requires identical stdout and exit status (QEMU's isa-debug-exit reports `(status << 1) | 1`, which the script decodes). If `qemu-system-x86_64` is missing or `/dev/kvm` is not accessible it prints `qemu: skipped` and only the golden files are checked. Golden files live in `tests/expected/` (`G.out` stdout, `G.rc` exit status).

## Bench

    make bench guests                      # build/bench/baseline_syscall and the guests
    bench/pick_cpu.sh                      # syscall time on each CPU; fastest is a proxy for a P-core
    bench/exit_cost.sh -c CPU -l LABEL     # pinned; add -n RUNS (default 5), -u for unpinned
    build/minivmm --ioeventfd --stats pio_loop.bin  # 0xE9 writes handled in the kernel, to compare against the exit path
    build/minivmm --stats guests.bin       # exit counts and ns in KVM_RUN / handler, on stderr

`exit_cost.sh` runs `pio_loop` (1M OUT to the 0xE9 sink), `mmio_loop` (1M reads of the unbacked VGA window) and `serial_loop` (200k bytes to 0x3F8, one `write(2)` each) after one discarded warm-up, and writes `bench/results/DATE-LABEL.txt` (machine, commit, command, per-run numbers, median/min/max). Numbers are ns per exit: wall is the whole loop divided by exit count, split into time inside `KVM_RUN` (guest entry/exit plus the kernel) and time in minivmm's handler. The remainder of wall is loop and timestamp overhead. `baseline_syscall` (raw `getppid`) gives the scale of one plain kernel crossing.

Caveat: under WSL2 KVM runs nested inside Hyper-V, so every exit costs far more than on bare metal and is noisy; compare runs from the same machine state, not absolute values to other systems.

## Environment

- KVM is required. Check it with `ls -l /dev/kvm`; your user must be in the `kvm` group (`sudo usermod -aG kvm $USER`, then log in again).
- WSL2 needs nested virtualization (Windows 11 or recent Windows 10, `nestedVirtualization=true` in `.wslconfig` on Hyper-V hosts that need it).
- `MINIVMM_KVM=/path` overrides the KVM device path (the negative tests use it to simulate a missing `/dev/kvm`).
- Keep the repo on the Linux filesystem (`~/code`), not under `/mnt/c`.
