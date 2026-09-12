#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0+

set -euo pipefail

srctree="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
test_output="$1"
mkdir -p "${test_output}"

pretty="${test_output}/pretty.txt"
targets="${test_output}/targets.txt"
expected="${test_output}/expected.txt"

make -C "${srctree}" list-defconfigs > "${pretty}"
make -C "${srctree}" list-defconfig-targets > "${targets}"

{
	for defconfig in "${srctree}"/{boards,soc}/*/configs/*_defconfig; do
		platform="$(basename -- "$(dirname -- "$(dirname -- "${defconfig}")")")"
		variant="$(basename -- "${defconfig}")"
		printf '%s_%s\n' "${platform}" "${variant}"
	done
} | sort > "${expected}"

diff -u "${expected}" "${targets}"
grep -qx 'Board:' "${pretty}"
grep -qx '  tinyvision:' "${pretty}"
grep -qx '    tinyvision_sram_defconfig' "${pretty}"
grep -qx 'SoC:' "${pretty}"
grep -qx '  sun8iw21_efex_defconfig' "${pretty}"

if grep -Eq '^  sun[^ ]*:$' "${pretty}"; then
	echo 'SoC defconfigs must not have an intermediate platform heading' >&2
	exit 1
fi
if grep -qx 'tinyvision_defconfig' "${targets}"; then
	echo 'SRAM defconfig targets must retain the sram variant' >&2
	exit 1
fi

# Configuration is parsed before the build graph.  A combined invocation must
# fail before it can leave a partially configured output tree behind.
mixed_out="$(mktemp -d "${TMPDIR:-/tmp}/syterkit-defconfig.XXXXXX")"
trap 'rm -rf "${mixed_out}"' EXIT
mixed_log="${test_output}/mixed-targets.log"
if make -C "${srctree}" --no-print-directory --silent \
	O="${mixed_out}" tinyvision_sram_defconfig all >"${mixed_log}" 2>&1; then
	echo 'configuration and build targets were unexpectedly accepted together' >&2
	exit 1
fi
if ! grep -Fq 'Cannot combine configuration goal(s)' "${mixed_log}"; then
	echo 'combined target failure did not explain how to recover' >&2
	sed -n '1,40p' "${mixed_log}" >&2
	exit 1
fi
if find "${mixed_out}" -type f -print -quit | grep -q .; then
	echo 'combined target failure created output files' >&2
	exit 1
fi

# Image aggregate targets must reject an unconfigured output tree before
# compiling host tools or standalone utilities.
for target in images image artifacts; do
	no_config_out="$(mktemp -d "${TMPDIR:-/tmp}/syterkit-no-config.XXXXXX")"
	no_config_log="${test_output}/no-config-${target}.log"
	if make -C "${srctree}" --no-print-directory --silent \
		O="${no_config_out}" "${target}" >"${no_config_log}" 2>&1; then
		echo "${target} unexpectedly succeeded without a configuration" >&2
		exit 1
	fi
	if ! grep -Fq 'No configuration found.' "${no_config_log}"; then
		echo "${target} failure did not explain how to configure the build" >&2
		sed -n '1,40p' "${no_config_log}" >&2
		exit 1
	fi
	if find "${no_config_out}" -type f -print -quit | grep -q .; then
		echo "${target} created output files without a configuration" >&2
		exit 1
	fi
	rm -rf "${no_config_out}"
done

echo 'TEST PASS defconfig_list'
