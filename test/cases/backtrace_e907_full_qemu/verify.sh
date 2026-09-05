#!/usr/bin/env bash

set -euo pipefail

data_dir="$1"
log="$2"
elf="$3"
nm="$4"
clean_log="$(mktemp)"
trap 'rm -f "${clean_log}"' EXIT
tr -d '\r' < "${log}" > "${clean_log}"

grep -Fqx "$(cat "${data_dir}/expected.txt")" "${clean_log}"
grep -Fqx 'Call trace:' "${clean_log}"
minimum_frames="$(tr -d '[:space:]' < "${data_dir}/minimum-frames.txt")"
actual_frames="$(grep -Ec '^ \[<0x[0-9a-f]{8}>\] [[:alnum:]_.$]+\+0x[0-9a-f]+/0x[0-9a-f]+$' "${clean_log}")"
if ((actual_frames < minimum_frames)); then
	echo "full E907 backtrace: found ${actual_frames} frames, expected ${minimum_frames}" >&2
	exit 1
fi
while read -r symbol; do
	grep -Eq "^ \[<0x[0-9a-f]{8}>\] ${symbol}\\+0x[0-9a-f]+/0x[0-9a-f]+$" "${clean_log}"
done < "${data_dir}/symbols.txt"
"${nm}" "${elf}" | grep -Eq '[[:space:]]__backtrace_symbols$'
if grep -Eq 'backtrace: failed|<unknown>|^backtrace: 0x' "${clean_log}"; then
	exit 1
fi
