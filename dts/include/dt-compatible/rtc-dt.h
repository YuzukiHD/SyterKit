/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_RTC_DT_H__
#define __DT_COMPATIBLE_RTC_DT_H__

#include <driver.h>
#include <drivers/rtc/rtc.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int
sunxi_rtc_dt_read_config(sunxi_rtc_t *rtc, int node) {
	const dt2c_fdt32_t *reg;
	sunxi_rtc_t config = {0};

	if (rtc == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_RTC_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	if (reg == NULL)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.data_base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	config.data_size = dt2c_fdt32_to_cpu(reg[1]);
	if (config.data_base == 0U ||
	    config.data_size < (RTC_BOOT_INDEX + 1U) * sizeof(uint32_t) ||
	    config.data_size % sizeof(uint32_t) != 0U)
		return DRIVER_ERROR_INVALID;

	*rtc = config;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_rtc_dt_read_alias(sunxi_rtc_t *rtc, const char *alias) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_RTC_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_rtc_dt_read_config(rtc, node);
}

#endif /* __DT_COMPATIBLE_RTC_DT_H__ */
