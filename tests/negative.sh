#!/usr/bin/env bash
# Failure cases: each must exit nonzero (or the given code) and print the expected message.
set -u
cd "$(dirname "$0")/.."
BIN=${BIN:-build/minivmm}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

fails=0
# expect NAME WANT_RC(any = nonzero) SUBSTRING CMD...
expect() {
	local name=$1 want=$2 sub=$3 err rc
	shift 3
	err=$(timeout 10 "$@" 2>&1 >/dev/null)
	rc=$?
	if { [ "$want" = any ] && [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ] || [ "$rc" = "$want" ]; } && [[ $err == *"$sub"* ]]; then
		echo "PASS $name"
	else
		echo "FAIL $name (rc=$rc: $err)"
		fails=$((fails + 1))
	fi
}

: >"$tmp/empty.bin"
truncate -s 2M "$tmp/big.bin"
printf 'bits 16\norg 0x7C00\nout 0x80, al\nhlt\n' >"$tmp/port.asm"
nasm -f bin -o "$tmp/port.bin" "$tmp/port.asm" || exit 1

MINIVMM_KVM=/nonexistent expect "no /dev/kvm" any "cannot open /nonexistent" "$BIN" build/guests/hello.bin
expect "missing file" any "cannot open $tmp/nope.bin" "$BIN" "$tmp/nope.bin"
expect "empty file" any "is empty" "$BIN" "$tmp/empty.bin"
expect "oversized image" any "larger than guest RAM" "$BIN" "$tmp/big.bin"
expect "unhandled port" any "unhandled I/O port 0x80" "$BIN" "$tmp/port.bin"
expect "no argument" 2 "usage:" "$BIN"

[ $fails -eq 0 ]
