#!/usr/bin/env bash
# A/B wall-time comparison of two binaries. Usage: bench/ab.sh -g GUEST -N EXITS [-n PAIRS] [-c CPU] [-l LABEL] [-f] BIN_A BIN_B
# Runs A,B,A,B,... (one discarded warm-up each) with stdout to /dev/null, no --stats, and times each run
# externally, so no instrumentation is in the measurement. The per-run time includes taskset/exec startup (same for A and B).
set -euo pipefail
set -f # BIN_A/BIN_B are word-split command strings; never glob them (wrappers like env are not supported: the sha256 line hashes the first word)
cd "$(dirname "$0")/.."
pairs=11 cpu=11 label=ab force=0 guest= exits=
cmdline="$0 $*"
usage() { echo "usage: $0 -g GUEST -N EXITS [-n PAIRS] [-c CPU] [-l LABEL] [-f] BIN_A BIN_B" >&2; exit 2; }
while getopts g:N:n:c:l:f o; do
	case $o in
	g) guest=$OPTARG ;; N) exits=$OPTARG ;; n) pairs=$OPTARG ;; c) cpu=$OPTARG ;; l) label=$OPTARG ;; f) force=1 ;;
	*) usage ;;
	esac
done
shift $((OPTIND - 1))
[ $# -eq 2 ] && [ -n "$guest" ] || usage
[[ $exits =~ ^[0-9]+$ ]] && [ "$exits" -ge 1 ] || { echo "-N must be an integer >= 1" >&2; usage; }
[[ $pairs =~ ^[0-9]+$ ]] && [ "$pairs" -ge 1 ] || { echo "-n must be an integer >= 1" >&2; usage; }
binA=$1 binB=$2
img=build/guests/$guest.bin
# BIN_A/BIN_B may be a command with arguments ('build/minivmm --ioeventfd'); the first word is the executable.
read -r exeA _ <<<"$binA"; read -r exeB _ <<<"$binB"
for f in "$exeA" "$exeB" "$img"; do [ -f "$f" ] || { echo "missing $f" >&2; exit 1; }; done
# Catch a wrong -N: loop guests do N iterations plus the final debug-exit.
n=$(sed -n 's/^%assign N \([0-9]*\).*/\1/p' "guests/$guest.asm" 2>/dev/null || true)
if [ -n "$n" ] && [ $((n + 1)) -ne "$exits" ]; then echo "-N $exits disagrees with %assign N $n (+1 exit) in guests/$guest.asm" >&2; exit 1; fi
out=bench/results/$(date +%F)-$label.txt
if [ -e "$out" ] && [ $force -eq 0 ]; then echo "$out exists; use -f to overwrite" >&2; exit 1; fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# One run: prints elapsed ns per exit. EPOCHREALTIME is "sec.usec"; dropping the dot gives microseconds.
once() {
	local s e
	s=${EPOCHREALTIME/./}
	taskset -c "$cpu" $1 "$img" >/dev/null
	e=${EPOCHREALTIME/./}
	awk -v us=$((e - s)) -v n="$exits" 'BEGIN{printf "%.1f", us * 1000 / n}'
}
stat3() { sort -n | awk '{a[NR]=$1} END{printf "%.1f %.1f %.1f", (NR%2?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2), a[1], a[NR]}'; }

once "$binA" >/dev/null; once "$binB" >/dev/null # warm-up
: >"$tmp/pairs"
for i in $(seq 1 "$pairs"); do
	a=$(once "$binA"); b=$(once "$binB")
	echo "$a $b" >>"$tmp/pairs"
done

if git diff --quiet HEAD; then dirty=clean; else dirty="dirty (diff sha256 $(git diff HEAD | sha256sum | cut -c1-16))"; fi
clock=$(cat /sys/devices/system/clocksource/clocksource0/current_clocksource 2>/dev/null || echo unknown)
nested=
grep -qw hypervisor /proc/cpuinfo && nested="environment: virtualized (hypervisor flag set; KVM here is nested virtualization)"
read -r amed amin amax <<<"$(awk '{print $1}' "$tmp/pairs" | stat3)"
read -r bmed bmin bmax <<<"$(awk '{print $2}' "$tmp/pairs" | stat3)"
read -r dmed dmin dmax <<<"$(awk '{print $2 - $1}' "$tmp/pairs" | stat3)"
# Exact two-sided sign test on the per-pair B-A differences: robust to the outlier runs that make a range-based rule useless.
read -r neg sign_p <<<"$(awk '{d = $2 - $1; if (d < 0) neg++; n++} END{k = (neg < n - neg ? neg : n - neg); t = 2 ^ -n; s = 0; for (i = 0; i <= k; i++) { s += t; t = t * (n - i) / (i + 1) } p = 2 * s; if (p > 1) p = 1; printf "%d %.4f", neg, p}' "$tmp/pairs")"
verdict=$(awk -v p="$sign_p" 'BEGIN{print (p < 0.05 ? "distinguishable from noise" : "within noise")}')

report() {
	echo "date: $(date -Is)"
	echo "commit: $(git rev-parse --short HEAD), tree $dirty"
	echo "cpu model: $(lscpu | sed -n 's/^Model name: *//p')"
	echo "kernel: $(uname -r)"
	echo "clocksource: $clock"
	[ -z "$nested" ] || echo "$nested"
	echo "pinned: cpu $cpu"
	echo "command: $cmdline"
	echo "guest: $guest, $exits exits per run, stdout to /dev/null, no --stats; wall time measured externally (includes exec startup, same for both)"
	echo "A: $binA sha256 $(sha256sum "$exeA" | cut -c1-16)"
	echo "B: $binB sha256 $(sha256sum "$exeB" | cut -c1-16)"
	echo "pairs: $pairs (after 1 discarded warm-up of each, alternating A,B)"
	echo
	echo "per pair, ns/exit (A, B, B-A)"
	awk '{printf "pair %d: A %.1f B %.1f diff %.1f\n", NR, $1, $2, $2 - $1}' "$tmp/pairs"
	echo
	echo "A ns/exit median/min/max: $amed $amin $amax"
	echo "B ns/exit median/min/max: $bmed $bmin $bmax"
	echo "B-A per-pair diff median/min/max: $dmed $dmin $dmax"
	echo "B faster in $neg of $pairs pairs; sign test p=$sign_p"
	echo "verdict: $verdict (p < 0.05 counts as distinguishable; median diff $dmed ns/exit)"
}
mkdir -p "$(dirname "$out")"
report >"$tmp/report"
mv "$tmp/report" "$out"
cat "$out"
echo "wrote $out" >&2
