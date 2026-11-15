#!/usr/bin/env bash
# Golden-file tests, plus a byte-for-byte QEMU comparison when QEMU and KVM are usable.
set -u
cd "$(dirname "$0")/.."
BIN=${BIN:-build/minivmm}
[ -f build/guests/hello.bin ] || make guests >/dev/null || exit 1

fails=0
check() { # check NAME STATUS(0=pass)
	if [ "$2" -eq 0 ]; then echo "PASS $1"; else echo "FAIL $1"; fails=$((fails + 1)); fi
}

qemu=0
if command -v qemu-system-x86_64 >/dev/null && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
	qemu=1
else
	echo "qemu: skipped"
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

for g in hello arith memtest exit_code; do
	img=build/guests/$g.bin
	timeout 10 "$BIN" "$img" >"$tmp/$g.out" 2>/dev/null
	rc=$?
	cmp -s "$tmp/$g.out" "tests/expected/$g.out" && [ "$rc" -eq "$(cat "tests/expected/$g.rc")" ]
	check "$g golden" $?
	[ "$qemu" -eq 1 ] || continue
	timeout 10 qemu-system-x86_64 -enable-kvm -cpu host -display none -serial stdio -no-reboot \
		-device isa-debug-exit,iobase=0xf4,iosize=0x01 -drive format=raw,file="$img" \
		>"$tmp/$g.qout" 2>/dev/null
	qrc=$?
	[ $((qrc & 1)) -eq 1 ] && qrc=$((qrc >> 1))
	cmp -s "$tmp/$g.out" "$tmp/$g.qout" && [ "$rc" -eq "$qrc" ]
	check "$g qemu" $?
done

err=$(timeout 10 "$BIN" build/guests/fault.bin 2>&1 >/dev/null)
rc=$?
[ "$rc" -ne 0 ] && [ "$rc" -ne 124 ] && [[ $err == *"triple fault"* ]]
check "fault triple fault" $?

err=$(timeout 10 "$BIN" --stats --ioeventfd build/guests/ioevent.bin 2>&1 >/dev/null)
rc=$?
[ "$rc" -eq 0 ] && grep -qx "ioeventfd signals 3" <<<"$err"
check "ioeventfd signals" $?

[ $fails -eq 0 ]
