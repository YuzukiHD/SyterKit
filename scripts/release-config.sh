#!/bin/sh
# SPDX-License-Identifier: GPL-2.0+

# Convert a normal Kconfig file into the configuration used by `make release`.
# Driver names are discovered from Kconfig itself, so adding a driver does not
# require another list in the top-level Makefile.
set -eu

if [ "$#" -ne 2 ]; then
	echo "usage: $0 <input-config> <output-config>" >&2
	exit 2
fi

input=$1
output=$2
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_root=$(CDPATH= cd -- "$script_dir/.." && pwd)

cp "$input" "$output"
sed -i -E \
	-e '/^(# )?CONFIG_BUILD_(RELEASE|DEBUG|TRACE)(=.*| is not set)?$/d' \
	-e '/^(# )?CONFIG_DRIVER_[A-Z0-9_]+_LOG_(GLOBAL|MUTE|ERROR|WARNING|INFO|DEBUG|TRACE|BACKTRACE)(=.*| is not set)?$/d' \
	"$output"

{
	echo 'CONFIG_BUILD_RELEASE=y'
	find "$source_root/drivers" -type f -name Kconfig -print0 |
		xargs -0 -r awk '
			/^[[:space:]]*config[[:space:]]+DRIVER_[A-Z0-9_]+_LOG_GLOBAL[[:space:]]*$/ {
				name = $2
				sub(/^DRIVER_/, "", name)
				sub(/_LOG_GLOBAL$/, "", name)
				print name
			}' |
		sort -u |
		while IFS= read -r driver; do
			[ -n "$driver" ] || continue
			printf 'CONFIG_DRIVER_%s_LOG_GLOBAL=y\n' "$driver"
		done
} >> "$output"
