#!/usr/bin/env bash

set -euo pipefail

data_dir="$1"
log="$2"
clean_log="$(mktemp)"
trap 'rm -f "${clean_log}"' EXIT
tr -d '\r' < "${log}" > "${clean_log}"
while read -r expected; do
	grep -Fqx "${expected}" "${clean_log}"
done < "${data_dir}/expected.txt"
if grep -Eq 'backtrace: failed|TEST FAIL' "${clean_log}"; then
	exit 1
fi
