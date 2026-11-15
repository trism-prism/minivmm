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

## Environment

- KVM is required. Check it with `ls -l /dev/kvm`; your user must be in the `kvm` group (`sudo usermod -aG kvm $USER`, then log in again).
- WSL2 needs nested virtualization (Windows 11 or recent Windows 10, `nestedVirtualization=true` in `.wslconfig` on Hyper-V hosts that need it).
- `MINIVMM_KVM=/path` overrides the KVM device path (the negative tests use it to simulate a missing `/dev/kvm`).
- Keep the repo on the Linux filesystem (`~/code`), not under `/mnt/c`.
