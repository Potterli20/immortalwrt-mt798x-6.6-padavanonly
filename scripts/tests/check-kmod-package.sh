#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Reject empty kmod packages: a KernelPackage whose KCONFIG/FILES gate is not
# satisfied silently produces an apk with only a .list file (see
# driver-reuse-assessment.md section 2.2 for the crypto-eip example).
#
# Usage: check-kmod-package.sh <path/to/kmod-*.apk> [more.apk ...]
set -eu

APK_BIN="${APK_BIN:-staging_dir/host/bin/apk}"
[ -x "$APK_BIN" ] || { echo "missing $APK_BIN" >&2; exit 2; }

rc=0
for pkg in "$@"; do
	[ -f "$pkg" ] || { echo "FAIL $pkg: not found"; rc=1; continue; }
	dump=$("$APK_BIN" adbdump "$pkg" 2>/dev/null)
	ko=$(printf '%s\n' "$dump" | grep -cE -- '- name: .*\.ko' || true)
	list=$(printf '%s\n' "$dump" | grep -cE -- '- name: .*\.list' || true)
	if [ "$ko" -lt 1 ]; then
		echo "FAIL $pkg: no .ko in package (empty kmod shell)"
		rc=1
	else
		echo "PASS $pkg: $ko kernel module(s), $list package list(s)"
	fi
done
exit "$rc"
