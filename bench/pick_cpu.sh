#!/usr/bin/env bash
# Time the syscall baseline on every CPU and print the fastest. The core type (P or E) is not
# exposed here, so this is an empirical proxy only: a faster syscall loop suggests a P-core.
set -eu
cd "$(dirname "$0")/.."
[ -x build/bench/baseline_syscall ] || make bench >/dev/null
best=; bestns=
for c in $(seq 0 $(($(nproc) - 1))); do
	ns=$(taskset -c "$c" build/bench/baseline_syscall 200000)
	echo "cpu $c: $ns ns"
	if [ -z "$bestns" ] || awk "BEGIN{exit !($ns < $bestns)}"; then best=$c bestns=$ns; fi
done
echo "fastest: cpu $best ($bestns ns)"
