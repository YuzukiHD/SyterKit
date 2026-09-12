#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0+

set -euo pipefail

srctree="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
test_output="$1"
mkdir -p "${test_output}"

# Reuse one output tree to exercise Kconfig transitions between every chip.
for defconfig in "${srctree}"/soc/*/configs/*_defconfig; do
	soc_name="$(basename -- "$(dirname -- "$(dirname -- "${defconfig}")")")"
	variant="$(basename -- "${defconfig}")"
	target="${soc_name}_${variant}"
	make -C "${srctree}" O="${test_output}" "${target}" > "${test_output}/config.log" 2>&1
	grep -qx "CONFIG_SYS_SOC=\"${soc_name}\"" "${test_output}/.config"
	grep -qx "CONFIG_SOC_${soc_name^^}=y" "${test_output}/.config"
	if grep -q '^CONFIG_BOARD_.*=y$' "${test_output}/.config"; then
		echo "${target} selected a board" >&2
		exit 1
	fi
	make -C "${srctree}" O="${test_output}" DT2C=/nonexistent/dt2c \
		-f Makefile -f test/cases/soc_efex/inspect.mk inspect-efex
	printf 'PASS %s\n' "${target}"
done

# Returning to a board config must clear the SoC application selection.
make -C "${srctree}" O="${test_output}" tinyvision_sram_defconfig > "${test_output}/config.log" 2>&1
grep -qx 'CONFIG_SYS_BOARD="tinyvision"' "${test_output}/.config"
if grep -qE '^CONFIG_(EFEX|EFEX_SOC_[A-Z0-9_]+)=y$|^CONFIG_SYS_SOC=' "${test_output}/.config"; then
	echo 'SoC eFEX selection leaked into the board configuration' >&2
	exit 1
fi
if make -C "${srctree}" O="${test_output}" tinyvision_efex_defconfig > "${test_output}/removed.log" 2>&1; then
	echo 'The removed board eFEX target is still accepted' >&2
	exit 1
fi
echo 'TEST PASS soc_efex'
