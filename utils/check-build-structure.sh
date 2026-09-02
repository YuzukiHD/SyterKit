#!/usr/bin/env bash

set -euo pipefail

srctree="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
third_party_dt2c="${srctree}/tools/dt2c"
rust_tree="${srctree}/rust"
status=0

fail() {
	echo "build structure: $*" >&2
	status=1
}

kconfig_symbols() {
	find "${srctree}" -path "${third_party_dt2c}" -prune -o \
		-type f -name 'Kconfig*' -exec sed -n -E \
		's/^[[:space:]]*(menuconfig|config)[[:space:]]+([^[:space:]]+).*/\2/p' {} + |
		sort -u
}

make_config_symbols() {
	find "${srctree}" \( -path "${srctree}/scripts/kconfig" \
		-o -path "${third_party_dt2c}" \) -prune -o \
		-type f \( -name Makefile -o -name '*.mk' \) -print0 |
		xargs -0 -r rg -o --no-filename --replace '$1' \
			'(?:^|[^A-Z0-9_])(CONFIG_[A-Z0-9_]+)' |
		sed 's/^CONFIG_//' | sort -u
}

defconfig_symbols() {
	find "${srctree}/configs" -type f -name '*_defconfig' -exec sed -n -E \
		-e 's/^CONFIG_([A-Z0-9_]+)=.*/\1/p' \
		-e 's/^# CONFIG_([A-Z0-9_]+) is not set$/\1/p' {} + |
		sort -u
}

doxygen_source_files() {
	find "${srctree}/arch" "${srctree}/boards" "${srctree}/core" \
		"${srctree}/drivers" "${srctree}/include" "${srctree}/lib" \
		"${srctree}/test" "${srctree}/tools" "${srctree}/utils" \
		\( -path "${third_party_dt2c}" \
			-o -path "${srctree}/include/lib/fdt" \
			-o -path "${srctree}/lib/fdt" \
			-o -path "${srctree}/include/lib/fatfs" \
			-o -path "${srctree}/lib/fatfs" \
			-o -path "${srctree}/boards/longanpi-3h/tinymaix" \) -prune \
		-o -path "${srctree}/test/out" -prune \
		-o -type f \( -name '*.c' -o -name '*.h' -o -name '*.S' \) -print0
}

mapfile -d '' -t doxygen_files < <(doxygen_source_files)

while IFS=: read -r source line; do
	[[ -n "${source}" ]] && fail "${source#${srctree}/}:${line}: Doxygen block has no @brief"
done < <(
	awk '
		function finish_block() {
			if (!member_doc && !has_brief)
				print FILENAME ":" start_line
			in_doc = 0
		}
		{
			if (!in_doc && match($0, /\/\*\*/)) {
				comment = substr($0, RSTART)
				in_doc = 1
				start_line = FNR
				member_doc = comment ~ /^\/\*\*</
				has_brief = comment ~ /@brief([[:space:]]|$)/
				if (comment ~ /\*\//)
					finish_block()
			} else if (in_doc) {
				if ($0 ~ /@brief([[:space:]]|$)/)
					has_brief = 1
				if ($0 ~ /\*\//)
					finish_block()
			}
		}
		END {
			if (in_doc)
				print FILENAME ":" start_line
		}
	' "${doxygen_files[@]}"
)

if rg -n 'Function name:|^[[:space:]]*\*?[[:space:]]*(Parameters|Returns?|Description):' \
		"${doxygen_files[@]}"; then
	fail "legacy documentation templates are not allowed"
fi

if rg -n '^[[:space:]]*\*[[:space:]]+@[A-Za-z_][A-Za-z0-9_]*:|^[[:space:]]*\*[[:space:]]+@param(\[[^]]+\])?[[:space:]]+[A-Za-z_][A-Za-z0-9_]*:' \
		"${doxygen_files[@]}"; then
	fail "Doxygen commands and parameter names must not use a trailing colon"
fi

if rg -n '^[[:space:]]*\*[[:space:]]+@param(\[[^]]+\])?[[:space:]]+(none|None|void|NULL|N/A)([.[:space:]]|$)|^[[:space:]]*\*[[:space:]]+@return[[:space:]]+(none|None|void|NULL|N/A)([.[:space:]]|$)' \
		"${doxygen_files[@]}"; then
	fail "void functions must not use placeholder @param or @return entries"
fi

if rg -n '^[[:space:]]*\*[[:space:]]+@(param(\[[^]]+\])?|return|retval)[[:space:]]*$' \
		"${doxygen_files[@]}"; then
	fail "Doxygen parameter and return commands require an inline description"
fi

if rg -n '^[[:space:]]*\*[[:space:]]+@macro([[:space:]]|$)|^[[:space:]]*\*[[:space:]]+@file[[:space:]]+[^@[:space:]]' \
		"${doxygen_files[@]}"; then
	fail "use @def for macros and argument-free @file commands"
fi

if grep -R -n -E --exclude-dir=dt2c --include='Kconfig*' \
		'^[[:space:]]*(default|def_bool)[[:space:]]+y([[:space:]]|$)' "${srctree}"; then
	fail "Kconfig must not enable options with default y"
fi

while IFS= read -r kconfig; do
	while read -r keyword symbol; do
		case "${symbol}" in
			DRIVER_*) ;;
			*) fail "${kconfig#${srctree}/}: ${keyword} ${symbol} must use DRIVER_*" ;;
		esac
	done < <(sed -n -E 's/^(menuconfig|config)[[:space:]]+([^[:space:]]+).*/\1 \2/p' "${kconfig}")
done < <(find "${srctree}/drivers" -type f -name Kconfig | sort)

while IFS= read -r symbol; do
	[[ -n "${symbol}" ]] && fail "Makefile references undefined Kconfig symbol CONFIG_${symbol}"
done < <(comm -23 <(make_config_symbols) <(kconfig_symbols))

while IFS= read -r symbol; do
	[[ -n "${symbol}" ]] && fail "defconfig references undefined Kconfig symbol CONFIG_${symbol}"
done < <(comm -23 <(defconfig_symbols) <(kconfig_symbols))

while IFS= read -r driver_dir; do
	relative_dir="${driver_dir#${srctree}/}"
	parent_dir="$(dirname -- "${driver_dir}")"
	base_dir="$(basename -- "${driver_dir}")"

	if [[ ! -f "${driver_dir}/Kconfig" ]]; then
		fail "${relative_dir} has no Kconfig"
	fi
	if [[ ! -f "${driver_dir}/Makefile" ]]; then
		fail "${relative_dir} has no Makefile"
	fi
	if [[ -f "${parent_dir}/Kconfig" ]] && \
			! grep -Fq "source \"${relative_dir}/Kconfig\"" "${parent_dir}/Kconfig"; then
		fail "${relative_dir}/Kconfig is not sourced by its parent"
	fi
	if [[ -f "${parent_dir}/Makefile" ]] && \
			! grep -Fq "${base_dir}/" "${parent_dir}/Makefile"; then
		fail "${relative_dir}/Makefile is not descended into by its parent"
	fi
done < <(find "${srctree}/drivers" -mindepth 1 -type d | sort)

while IFS= read -r board_dir; do
	relative_dir="${board_dir#${srctree}/}"
	[[ -f "${board_dir}/Kconfig" ]] || fail "${relative_dir} has no Kconfig"
	[[ -f "${board_dir}/Makefile" ]] || fail "${relative_dir} has no Makefile"
	[[ -f "${board_dir}/board.dts" ]] || fail "${relative_dir} has no board.dts"
	board_name="$(basename -- "${board_dir}")"
	if [[ ! -d "${srctree}/configs/${board_name}" ]] ||
	   ! find "${srctree}/configs/${board_name}" -maxdepth 1 -type f \
		\( -name '*_sram_defconfig' -o -name 'sram_defconfig' \) \
		-print -quit | grep -q .; then
		fail "${relative_dir} has no matching defconfig"
	fi
done < <(find "${srctree}/boards" -mindepth 1 -maxdepth 1 -type d | sort)

while IFS= read -r defconfig; do
	name="${defconfig#${srctree}/configs/}"
	board="$(basename -- "$(dirname -- "${defconfig}")")"
	variant="${defconfig##*/}"
	variant="${variant%_defconfig}"
	mode="${variant##*_}"
	arch_count="$(sed -n -E '/^CONFIG_ARCH_(ARM32|RISCV32|RISCV64)=y$/p' \
		"${defconfig}" | wc -l)"
	core_count="$(sed -n -E \
		'/^CONFIG_(CPU_(CORTEX_A7|ARMV8|ARMV8_2)|ARCH_CPU_(E907|C907|C906))=y$/p' \
		"${defconfig}" | wc -l)"
	build_mode_count="$(sed -n -E \
		'/^CONFIG_BUILD_(RELEASE|DEBUG|TRACE)=y$/p' \
		"${defconfig}" | wc -l)"
	gpio_count="$(sed -n -E '/^CONFIG_DRIVER_GPIO_V(1|2(_POW)?|3|4)=y$/p' \
		"${defconfig}" | wc -l)"
	board_count="$(sed -n -E '/^CONFIG_BOARD_[A-Z0-9_]+=y$/p' \
		"${defconfig}" | wc -l)"
	value_count="$(sed -n -E \
		'/^CONFIG_(SYS_BOARD|SPL_BIN_TEXT_BASE|SPL_BIN_MAX_SIZE|SPL_FEL_TEXT_BASE|SPL_FEL_MAX_SIZE)=/p' \
		"${defconfig}" | wc -l)"
	app_mode_count="$(sed -n -E '/^CONFIG_(APP_SRAM|APP_DRAM|EFEX)=y$/p' \
		"${defconfig}" | wc -l)"
	configured_board="$(sed -n -E 's/^CONFIG_SYS_BOARD="([^"]+)"$/\1/p' \
		"${defconfig}")"
	configured_mode="$(sed -n -E \
		's/^CONFIG_(APP_SRAM|APP_DRAM|EFEX)=y$/\1/p' "${defconfig}")"

	[[ "${arch_count}" -eq 1 ]] || fail "${name} must select exactly one architecture"
	[[ "${core_count}" -eq 1 ]] || fail "${name} must select exactly one processor core"
	[[ "${build_mode_count}" -eq 1 ]] || fail "${name} must select exactly one build mode"
	[[ "${gpio_count}" -eq 1 ]] || fail "${name} must select exactly one GPIO controller"
	[[ "${board_count}" -eq 1 ]] || fail "${name} must select exactly one board"
	[[ "${value_count}" -eq 5 ]] || fail "${name} must define all board image parameters"
	[[ "${app_mode_count}" -eq 1 ]] || fail "${name} must select exactly one application mode"
	case "${mode}" in
		sram) expected_mode="APP_SRAM" ;;
		dram) expected_mode="APP_DRAM" ;;
		efex) expected_mode="EFEX" ;;
		*) expected_mode=""; fail "${name} has unknown application mode ${mode}" ;;
	esac
	[[ "${configured_mode}" == "${expected_mode}" ]] || \
		fail "${name} must select CONFIG_${expected_mode}"
	if grep -q '^CONFIG_APP_DRAM=y$' "${defconfig}"; then
		dram_value_count="$(sed -n -E \
			'/^CONFIG_APP_DRAM_(TEXT_BASE|MAX_SIZE)=/p' "${defconfig}" | wc -l)"
		[[ "${dram_value_count}" -eq 2 ]] || \
			fail "${name} must define the DRAM application window"
	fi
	[[ "${configured_board}" == "${board}" ]] || \
		fail "${name} CONFIG_SYS_BOARD must be \"${board}\""
done < <(find "${srctree}/configs" -type f -name '*_defconfig' | sort)

if ! grep -Fqx 'source "$(BOARD_KCONFIG_LIST)"' "${srctree}/boards/Kconfig" || \
		! grep -Fq '$(wildcard $(srctree)/boards/*/Kconfig)' "${srctree}/Makefile"; then
	fail "board Kconfig discovery must remain automatic"
fi

if ! grep -Fqx 'arch_inc := arch/$(arch_dir)/include' "${srctree}/Makefile" || \
		! grep -Fqx 'include_dirs := include $(arch_inc)' "${srctree}/Makefile"; then
	fail "top-level include paths must contain only include and the selected arch include"
fi

if grep -n -E -- '-m(cpu|arch|abi|fpu|float-abi)(=|[[:space:]])' "${srctree}/Makefile"; then
	fail "CPU and ABI flags belong in arch/*/Makefile"
fi

if grep -R -n --exclude-dir=dt2c --include='Makefile' --include='*.mk' --include='*.c' \
		--include='*.h' --include='*.S' -- '-Wno-' "${srctree}"; then
	fail "warning suppression flags are not allowed"
fi

for forbidden_dir in linux sunxi sstdlib cmake; do
	if find "${srctree}" -path "${third_party_dt2c}" -prune -o \
			-type d -name "${forbidden_dir}" -print -quit | grep -q .; then
		fail "forbidden directory name: ${forbidden_dir}"
	fi
done

if find "${srctree}" \( -path "${third_party_dt2c}" -o -path "${rust_tree}" \) -prune -o \
		-type f \( -name CMakeLists.txt -o -name '*.cmake' \
		-o -name Cargo.toml -o -name '*.rs' \) \
		! -path "${srctree}/boards/*/*/*/main.rs" -print -quit | grep -q .; then
	fail "CMake and Rust build files are not allowed"
fi

if grep -R -n -E --exclude-dir=dt2c --include='Makefile' --include='*.mk' --include='*.c' \
		--include='*.h' --include='*.S' \
		'(^|[/<])sstdlib([/.>]|$)|include/linux|<linux/' "${srctree}"; then
	fail "legacy or Linux header paths are not allowed"
fi

exit "${status}"
