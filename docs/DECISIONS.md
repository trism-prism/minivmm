# Decisions

Two kinds of entry. **Design choices** are one line each. **Measured changes** use four fixed lines: Tried, Before, After, Not measured. Raw numbers live in `bench/results/`; every result file records its commit, flags and machine, and `docs/RUNNING.md` has the commands that produced it.

## Environment (applies to every number below)

- Intel i7-1355U laptop, 12 logical CPUs, WSL2 kernel `6.18.33.2-microsoft-standard-WSL2`. KVM here is **nested** under Hyper-V, so every exit is much slower than on bare metal (about 5 µs here). Only before/after differences on this machine are claimed, never absolute figures.
- The CPU is hybrid (P and E cores) but WSL does not show which is which, and `bench/pick_cpu.sh` found no difference (every CPU 90 to 105 ns per `getppid`). Pinned runs use CPU 11, an arbitrary choice.
- Clock: `tsc`. `clock_gettime(CLOCK_MONOTONIC_RAW)` costs 22 to 32 ns, and each exit takes four reads, so the `--stats` handler column includes roughly one clock read of overhead.
- Run-to-run spread is 7 to 9% with occasional slow outliers (up to +70%). The machine also drifts between runs made minutes apart, so numbers from different result files are not directly comparable. Only the alternating A/B runs are.

## Design choices

- **Two memory slots, hole at 0xA0000-0xBFFFF.** One 1 MiB slot would back the VGA window with plain RAM, so the MMIO probe could never fire. Found while writing the devices.
- **1 MiB guest RAM**, one anonymous `mmap`, loader rejects images over `RAM - 0x7C00`.
- **Every guest is a 512-byte boot sector** (`guests/boot.inc`) so QEMU and minivmm run the identical file. Guests end by writing port `0xF4`, never HLT, because QEMU does not stop on HLT.
- **QEMU exit codes** are `(status << 1) | 1`. `run.sh` decodes odd codes, which cannot tell guest status 0 from a QEMU failure (both give 1). The output comparison covers it for guests that print.
- **Process exit code is the guest's status.** A triple fault also exits 1, so tests tell it apart by the stderr message.
- **Serial and debug-exit accept byte-wide accesses only.** Wider ones report as unhandled rather than silently dropping bytes.
- **Port `0xE9` is a write-only discard port** so the port-I/O benchmark measures the exit and not a `write(2)`.
- **`MINIVMM_KVM` overrides the device path**, used only to test the no-`/dev/kvm` failure.
- **`KIOCTL` is a GNU C statement expression.** The code is C11 plus that extension, accepted by gcc and clang.
- **A/B verdict is an exact sign test** on per-pair differences (p < 0.05 counts as distinguishable). My first rule compared the median difference with the range of run A, which a couple of outliers made useless.

## Measured changes

### Baseline (reference, not a change)
Tried: `bench/exit_cost.sh`, 5 runs after a warm-up, `--stats`, pinned to CPU 11 (`2026-11-15-baseline.txt`).
Before: n/a.
After: port I/O exit 5648 ns, MMIO exit 6577 ns, serial exit 5668 ns. `KVM_RUN` is about 98% of that. My handler is about 30 ns (sink), 46 ns (MMIO), 226 ns (serial `write(2)`). One `getppid` syscall is about 108 ns.
Not measured: why MMIO costs about 1 µs more than port I/O; any bare-metal figure.

### Pinning versus not pinning
Tried: the same bench with and without `taskset` (`baseline` and `baseline-unpinned`).
Before: unpinned medians 5430 / 6524 / 6029 ns (pio / mmio / serial).
After: pinned medians 5648 / 6577 / 5668 ns.
Not measured: any difference. Pinned is better for serial and worse for pio, within the spread, from one 5-run set each. No pinning claim is supported. Which core type CPU 11 is.

### Buffer serial output (4 KiB) instead of one `write(2)` per byte
Tried: `serial.c` buffers and flushes when full and once after the run (and via `atexit`, so `die()` paths flush too).
Before: serial handler 226 ns per exit (range 211 to 238).
After: serial handler 30 ns (range 28 to 30). The ranges do not overlap. In the alternating A/B (51 pairs, `2026-11-15-serial-buffer-ab.txt`) wall time per exit fell by a median of 156 ns (about 3%), faster in 34 of 51 pairs, sign test p = 0.024. An earlier 51-pair run of the same two binaries gave 241 ns, 36 of 51, p = 0.005.
Not measured: bare metal. A guest that prints more per exit. The cost of the one final flush (outside the stats timing). The trade-off is that output of a killed or endlessly looping guest is lost until 4 KiB accumulates.

### Take exit timestamps only with `--stats`
Tried: `stats_*` return immediately unless `--stats` was given, so a normal run takes no clock reads (four per exit before, 22 to 32 ns each).
Before / After: alternating A/B on `pio_loop`, 41 pairs, `2026-11-15-stats-gate-ab.txt`. Median wall per exit 5853 ns before, 5815 ns after. Median per-pair difference -100 ns (about 2%), faster in 22 of 41 pairs, sign test p = 0.76: **not distinguishable from noise**.
Not measured: any real gain. The expected saving (about 100 ns, under 2%) is below what this machine can resolve with 41 pairs; this run also had outliers up to 11.5 µs. I kept the change anyway because it is simpler to reason about (no instrumentation when not asked for) and cannot slow anything down, but it is recorded as a no-measured-win.

### `--ioeventfd`: complete the sink-port write in the kernel
Tried: opt-in `--ioeventfd` registers `KVM_IOEVENTFD` (PIO, port `0xE9`, no datamatch) so a guest write signals an eventfd and never returns to my process. The eventfd counter is read after the run to check no write was lost (1,000,000 writes gave 1,000,000 signals).
Before: `pio_loop`, same binary without the flag, alternating A/B, 21 pairs, `2026-11-15-ioeventfd-ab.txt`. Median 5205 ns per write.
After: with the flag, median 4142 ns per write. Median per-pair difference -1105 ns (about 21%), faster in 21 of 21 pairs, p < 0.0001: **distinguishable from noise**, and the only one of the four changes that moves the dominant cost.
Not measured: bare metal (the saving is the user-space round trip; how much of that survives without nested virtualization is unknown). MMIO exits (`KVM_IOEVENTFD` supports MMIO too, not attempted). Any use where the VMM needs the written data: an ioeventfd only counts writes, so it fits a doorbell or the bench sink, not the serial port.
Correction to an earlier note: one pinned single run with `--stats` measured 5272 ns per write with the flag and I briefly read that as "no gain". It was one sample taken between drifting runs. The alternating A/B above is the measurement that counts.
