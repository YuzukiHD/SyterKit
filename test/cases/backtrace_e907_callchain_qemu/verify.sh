#!/usr/bin/env bash

set -euo pipefail

data_dir="$1"
log="$2"
elf="$3"
addr2line="$5"
clean_log="$(mktemp)"
trap 'rm -f "${clean_log}"' EXIT
tr -d '\r' < "${log}" > "${clean_log}"

grep -Fqx "$(cat "${data_dir}/expected.txt")" "${clean_log}"
minimum_frames="$(tr -d '[:space:]' < "${data_dir}/minimum-frames.txt")"
actual_frames="$(grep -c 'backtrace: 0x' "${clean_log}")"
if ((actual_frames < minimum_frames)); then
	echo "backtrace test: found ${actual_frames} frames, expected ${minimum_frames}" >&2
	exit 1
fi

symbols="$(mktemp)"
trap 'rm -f "${clean_log}" "${symbols}"' EXIT
while read -r address; do
	"${addr2line}" -f -e "${elf}" "${address}" | sed -n '1p'
done < <(grep -o '0x[0-9a-fA-F]*' "${clean_log}") > "${symbols}"
while read -r symbol; do
	grep -Fqx "${symbol}" "${symbols}"
done < "${data_dir}/symbols.txt"
if grep -Fq 'backtrace: failed' "${clean_log}"; then
	exit 1
fi
