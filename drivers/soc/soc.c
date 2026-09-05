/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "soc: " fmt

/**
 * @file soc.c
 * @brief Allwinner SoC identification driver.
 *
 * Resolves the SoC register window from the allwinner,soc device-tree node
 * and exposes the die/batch platform identifier latched in the upper 16 bits
 * of the version register.
 */

#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <driver.h>
#include <dt2c/driver.h>

#include <drivers/soc/soc.h>
#include <dt-compatible/soc-dt.h>

#define SUNXI_SOC_VERSION_OFFSET  0x24U
#define SUNXI_SOC_DIE_ENABLE_BIT  (1u << 15)
#define SUNXI_SOC_DIE_INFO_MASK   0xFFFF0000U

static sunxi_soc_t soc_descriptor;
static bool soc_descriptor_valid;

/**
 * @brief Locate and parse the allwinner,soc device-tree node.
 *
 * Finds the first available allwinner,soc node and caches its register
 * window for subsequent calls.
 *
 * @return DRIVER_OK when the descriptor is ready, DRIVER_ERROR_INVALID
 *         otherwise.
 */
static int sunxi_soc_dt_init(void)
{
	int node;

	if (soc_descriptor_valid)
		return DRIVER_OK;
	node = dt2c_fdt_node_offset_by_compatible(DT2C_FDT_COMPILED_TREE, 0, SUNXI_SOC_COMPATIBLE);
	if (node < 0 || sunxi_soc_dt_read_config(&soc_descriptor, node) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	soc_descriptor_valid = true;
	return DRIVER_OK;
}

uint32_t sunxi_soc_platform_id(void)
{
	uintptr_t reg;
	uint32_t value;

	if (sunxi_soc_dt_init() != DRIVER_OK || SUNXI_SOC_VERSION_OFFSET >= soc_descriptor.size)
		return 0U;

	reg = soc_descriptor.base + SUNXI_SOC_VERSION_OFFSET;
	value = read32(reg);
	value |= SUNXI_SOC_DIE_ENABLE_BIT;
	write32(reg, value); /* write back so the die info latches into the high 16 bits */
	value = read32(reg);

	return value & SUNXI_SOC_DIE_INFO_MASK;
}

DT2C_DRIVER_COMPAT("allwinner,soc");
