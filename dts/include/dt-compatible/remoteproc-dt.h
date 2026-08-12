/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_REMOTEPROC_DT_H__
#define __DT_COMPATIBLE_REMOTEPROC_DT_H__

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_REMOTEPROC_SUN20IW1_HIFI4_COMPATIBLE \
	"allwinner,sun20iw1-hifi4"
#define SUNXI_REMOTEPROC_SUN300IW1_A27L2_COMPATIBLE \
	"allwinner,sun300iw1-a27l2"
#define SUNXI_REMOTEPROC_SUN50IW10_AR100_COMPATIBLE \
	"allwinner,sun50iw10-ar100"
#define SUNXI_REMOTEPROC_SUN55IW3_E906_COMPATIBLE \
	"allwinner,sun55iw3-e906"
#define SUNXI_REMOTEPROC_SUN8IW20_HIFI4_COMPATIBLE \
	"allwinner,sun8iw20-hifi4"
#define SUNXI_REMOTEPROC_SUN8IW20_C906_COMPATIBLE \
	"allwinner,sun8iw20-c906"
#define SUNXI_REMOTEPROC_SUN8IW21_E907_COMPATIBLE \
	"allwinner,sun8iw21-e907"

static inline __attribute__((always_inline)) sunxi_remoteproc_variant_t
sunxi_remoteproc_dt_variant(int node) {
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN20IW1_HIFI4_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN20IW1_HIFI4;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN300IW1_A27L2_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN300IW1_A27L2;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN50IW10_AR100_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN50IW10_AR100;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN55IW3_E906_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN55IW3_E906;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN8IW20_HIFI4_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN8IW20_HIFI4;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN8IW20_C906_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN8IW20_C906;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_REMOTEPROC_SUN8IW21_E907_COMPATIBLE) == 0)
		return SUNXI_REMOTEPROC_VARIANT_SUN8IW21_E907;
	return SUNXI_REMOTEPROC_VARIANT_INVALID;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_string(int node, const char *property,
				   const char **value) {
	const char *string;
	int length;

	string = (const char *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, property, &length);
	if (string == NULL || length <= 1 || string[length - 1] != '\0' ||
	    __builtin_strlen(string) != (size_t) length - 1U)
		return false;
	*value = string;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_registers(int node, sunxi_remoteproc_t *remoteproc) {
	const dt2c_fdt32_t *cells;
	size_t count;
	size_t index;
	int length;

	cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "reg", &length);
	if (cells == NULL || length <= 0 ||
	    length % (2 * (int) sizeof(*cells)) != 0)
		return false;
	count = (size_t) length / (2U * sizeof(*cells));
	if (count == 0U || count > SUNXI_REMOTEPROC_MAX_REGISTERS)
		return false;

	for (index = 0U; index < count; ++index) {
		uint32_t base = dt2c_fdt32_to_cpu(cells[index * 2U]);
		uint32_t size = dt2c_fdt32_to_cpu(cells[index * 2U + 1U]);

		if (base == 0U || size == 0U || base + size < base)
			return false;
		remoteproc->registers[index].base = (uintptr_t) base;
		remoteproc->registers[index].size = (size_t) size;
	}
	remoteproc->register_count = count;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_region(int node, sunxi_remoteproc_firmware_t *firmware) {
	const dt2c_fdt32_t *reg;
	uint32_t address;
	uint32_t size;

	if (node < 0 || !syterkit_dt_node_available(node))
		return false;
	reg = syterkit_dt_cells(node, "reg", 2);
	if (reg == NULL)
		return false;
	address = dt2c_fdt32_to_cpu(reg[0]);
	size = dt2c_fdt32_to_cpu(reg[1]);
	if (address == 0U || size == 0U || address + size < address)
		return false;
	firmware->load_address = (uintptr_t) address;
	firmware->region_size = (size_t) size;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_primary_firmware(int node,
				     sunxi_remoteproc_t *remoteproc) {
	int region;

	if (!sunxi_remoteproc_dt_string(node, "firmware-name",
					&remoteproc->firmware[0].name))
		return false;
	region = syterkit_dt_phandle_node(node, "memory-region", NULL);
	if (!sunxi_remoteproc_dt_region(region, &remoteproc->firmware[0]))
		return false;
	remoteproc->firmware_count = 1U;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_auxiliary_firmware(int node,
				       sunxi_remoteproc_t *remoteproc) {
	const dt2c_fdt32_t *phandles;
	const char *name;
	int count;
	int length;
	int region;
	int index;

	phandles = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,auxiliary-memory-regions", &length);
	if (phandles == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
		count = dt2c_fdt_stringlist_count(
				DT2C_FDT_COMPILED_TREE, node,
				"allwinner,auxiliary-firmware-names");
		return count == -DT2C_FDT_ERR_NOTFOUND;
	}
	if (phandles == NULL || length <= 0 ||
	    length % (int) sizeof(*phandles) != 0)
		return false;
	count = length / (int) sizeof(*phandles);
	if (count >= (int) SUNXI_REMOTEPROC_MAX_FIRMWARES ||
	    dt2c_fdt_stringlist_count(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,auxiliary-firmware-names") != count)
		return false;

	for (index = 0; index < count; ++index) {
		name = dt2c_fdt_stringlist_get(
				DT2C_FDT_COMPILED_TREE, node,
				"allwinner,auxiliary-firmware-names", index,
				&length);
		region = dt2c_fdt_node_offset_by_phandle(
				DT2C_FDT_COMPILED_TREE,
				dt2c_fdt32_to_cpu(phandles[index]));
		if (name == NULL || length <= 0 ||
		    !sunxi_remoteproc_dt_region(
				    region, &remoteproc->firmware[index + 1U]))
			return false;
		remoteproc->firmware[index + 1U].name = name;
	}
	remoteproc->firmware_count += (size_t) count;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_format(int node, sunxi_remoteproc_t *remoteproc) {
	const char *format;

	if (!sunxi_remoteproc_dt_string(node, "allwinner,firmware-format",
					&format))
		return false;
	if (__builtin_strcmp(format, "elf32") == 0)
		remoteproc->format = SUNXI_REMOTEPROC_FIRMWARE_ELF32;
	else if (__builtin_strcmp(format, "elf64") == 0)
		remoteproc->format = SUNXI_REMOTEPROC_FIRMWARE_ELF64;
	else if (__builtin_strcmp(format, "raw") == 0)
		remoteproc->format = SUNXI_REMOTEPROC_FIRMWARE_RAW;
	else
		return false;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_entry(int node, sunxi_remoteproc_t *remoteproc) {
	const dt2c_fdt32_t *entry;
	int length;

	entry = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,entry-address", &length);
	if (entry == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
		if (remoteproc->format == SUNXI_REMOTEPROC_FIRMWARE_RAW)
			return false;
		remoteproc->entry_from_elf = true;
		return true;
	}
	if (entry == NULL || length != (int) sizeof(*entry))
		return false;
	remoteproc->entry = (uintptr_t) dt2c_fdt32_to_cpu(entry[0]);
	remoteproc->entry_from_elf = false;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_address_map(int node,
				sunxi_remoteproc_t *remoteproc) {
	const dt2c_fdt32_t *cells;
	size_t count;
	size_t index;
	int length;

	cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,address-map", &length);
	if (cells == NULL && length == -DT2C_FDT_ERR_NOTFOUND)
		return remoteproc->format != SUNXI_REMOTEPROC_FIRMWARE_RAW;
	if (cells == NULL || length <= 0 ||
	    length % (3 * (int) sizeof(*cells)) != 0)
		return false;
	count = (size_t) length / (3U * sizeof(*cells));
	if (count > SUNXI_REMOTEPROC_MAX_ADDRESS_MAPS)
		return false;

	for (index = 0U; index < count; ++index) {
		sunxi_remoteproc_address_map_t *range =
				&remoteproc->address_map[index];

		range->device_start =
				(uintptr_t) dt2c_fdt32_to_cpu(cells[index * 3U]);
		range->device_end =
				(uintptr_t) dt2c_fdt32_to_cpu(cells[index * 3U + 1U]);
		range->physical_start =
				(uintptr_t) dt2c_fdt32_to_cpu(cells[index * 3U + 2U]);
		if (range->device_start > range->device_end ||
		    range->physical_start == 0U ||
		    (index != 0U &&
		     range->device_start <=
			     remoteproc->address_map[index - 1U].device_end))
			return false;
	}
	remoteproc->address_map_count = count;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_remoteproc_dt_rtc(int node, sunxi_remoteproc_t *remoteproc,
			sunxi_rtc_t *rtc) {
	const dt2c_fdt32_t *phandle;
	int length;
	int rtc_node;

	phandle = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "allwinner,rtc", &length);
	if (phandle == NULL && length == -DT2C_FDT_ERR_NOTFOUND)
		return rtc == NULL;
	if (phandle == NULL || length != (int) sizeof(*phandle) || rtc == NULL)
		return false;
	rtc_node = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (rtc_node < 0 || rtc->dt_node != rtc_node ||
	    !syterkit_dt_node_available(rtc_node) ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, rtc_node,
			SUNXI_RTC_COMPATIBLE) != 0)
		return false;
	remoteproc->rtc = rtc;
	return true;
}

static inline __attribute__((always_inline)) int
sunxi_remoteproc_dt_read_config(sunxi_remoteproc_t *remoteproc, int node,
					sunxi_rtc_t *rtc) {
	sunxi_remoteproc_t config = {0};
	sunxi_remoteproc_variant_t variant;

	variant = sunxi_remoteproc_dt_variant(node);
	if (remoteproc == NULL || node < 0 ||
	    variant == SUNXI_REMOTEPROC_VARIANT_INVALID ||
	    !syterkit_dt_node_available(node) ||
	    !sunxi_remoteproc_dt_registers(node, &config) ||
	    !sunxi_remoteproc_dt_primary_firmware(node, &config) ||
	    !sunxi_remoteproc_dt_auxiliary_firmware(node, &config) ||
	    !sunxi_remoteproc_dt_format(node, &config) ||
	    !sunxi_remoteproc_dt_entry(node, &config) ||
	    !sunxi_remoteproc_dt_address_map(node, &config) ||
	    !sunxi_remoteproc_dt_rtc(node, &config, rtc))
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	if (sunxi_remoteproc_bind(&config, variant) != DRIVER_OK ||
	    config.ops == NULL)
		return DRIVER_ERROR_INVALID;
	*remoteproc = config;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_remoteproc_dt_read_alias(sunxi_remoteproc_t *remoteproc,
			       const char *alias, sunxi_rtc_t *rtc) {
	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_remoteproc_dt_read_config(
			remoteproc, syterkit_dt_alias_node(alias, NULL), rtc);
}

#endif /* __DT_COMPATIBLE_REMOTEPROC_DT_H__ */
