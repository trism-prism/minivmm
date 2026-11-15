#!/usr/bin/env bash
# Exit-cost benchmark. Usage: bench/exit_cost.sh [-n RUNS] [-c CPU] [-u] [-l LABEL] [-f]
# -u runs unpinned; otherwise pinned to CPU (default 0). -f overwrites an existing results file.
# Writes bench/results/DATE-LABEL.txt only if every run succeeded.
set -euo pipefail
cd "$(dirname "$0")/.."
runs=5 cpu=0 pin=1 label=run force=0
cmdline="$0 $*"
usage() { echo "usage: $0 [-n RUNS] [-c CPU] [-u] [-l LABEL] [-f]" >&2; exit 2; }
while getopts n:c:ul:f o; do
	case $o in
	n) runs=$OPTARG ;; c) cpu=$OPTARG ;; u) pin=0 ;; l) label=$OPTARG ;; f) force=1 ;;
	*) usage ;;
	esac
done
[[ $runs =~ ^[0-9]+$ ]] && [ "$runs" -ge 1 ] || { echo "-n must be an integer >= 1" >&2; usage; }
out=bench/results/$(date +%F)-$label.txt
if [ -e "$out" ] && [ $force -eq 0 ]; then echo "$out exists; use -f to overwrite" >&2; exit 1; fi

make --no-print-directory all guests bench >/dev/null
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
wrap=(); where=unpinned
if [ $pin -eq 1 ]; then wrap=(taskset -c "$cpu"); where="cpu $cpu"; fi

# median/min/max of stdin numbers, as "median min max"
stat3() { sort -n | awk '{a[NR]=$1} END{printf "%.1f %.1f %.1f", (NR%2?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2), a[1], a[NR]}'; }

if git diff --quiet HEAD; then dirty=clean; else dirty="dirty (diff sha256 $(git diff HEAD | sha256sum | cut -c1-16))"; fi
clock=$(cat /sys/devices/system/clocksource/clocksource0/current_clocksource 2>/dev/null || echo unknown)
cflags=$(make -s -p -n 2>/dev/null | sed -n 's/^CFLAGS *[:?]*= *//p' | head -1)
nested=
grep -qw hypervisor /proc/cpuinfo && nested="environment: virtualized (hypervisor flag set; KVM here is nested virtualization)"
clk=$("${wrap[@]}" build/bench/clock_cost)

report() {
	echo "date: $(date -Is)"
	echo "commit: $(git rev-parse --short HEAD), tree $dirty"
	echo "cpu model: $(lscpu | sed -n 's/^Model name: *//p')"
	echo "kernel: $(uname -r)"
	echo "clocksource: $clock"
	echo "CFLAGS: $cflags"
	[ -z "$nested" ] || echo "$nested"
	echo "pinned: $where"
	echo "command: $cmdline"
	echo "runs: $runs (after 1 discarded warm-up)"
	echo "clock_gettime ns/call: $clk"
	echo
	echo "baseline getppid syscall, median of 5, ns/call: $("${wrap[@]}" build/bench/baseline_syscall)"
	echo "note: each exit takes 4 clock_gettime calls, about 4 x $clk ns of the wall-minus-parts gap and of the handler figure."
	echo
	echo "per run: wall, KVM_RUN and handler ns per exit (wall = whole loop incl. timestamp/loop overhead)"
	for g in pio_loop mmio_loop serial_loop; do
		echo "== $g"
		: >"$tmp/$g"
		for i in $(seq 0 "$runs"); do
			s=$("${wrap[@]}" build/minivmm --stats build/guests/$g.bin 2>&1 >/dev/null) || { echo "$g failed: $s" >&2; exit 1; }
			line=$(echo "$s" | awk '/^total_exits/{n=$2} /^wall_ns/{w=$2} /^kvm_run_ns/{k=$2} /^handler_ns/{h=$2} END{if (n > 0) printf "%d %.1f %.1f %.1f", n, w/n, k/n, h/n}')
			[ -n "$line" ] || { echo "$g reported no exits: $s" >&2; exit 1; }
			[ "$i" -eq 0 ] && continue # warm-up
			echo "$line" >>"$tmp/$g"
			read -r n w k h <<<"$line"
			echo "run $i: exits $n wall $w kvm_run $k handler $h"
		done
		for col in 2 3 4; do
			name=(x x wall kvm_run handler); name=${name[$col]}
			echo "$name ns/exit median/min/max: $(awk -v c=$col '{print $c}' "$tmp/$g" | stat3)"
		done
	done
}

report >"$tmp/report"
mkdir -p "$(dirname "$out")"
mv "$tmp/report" "$out"
cat "$out"
echo "wrote $out" >&2
