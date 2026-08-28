/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SPIF_NOR_DT_H__
#define __DT_COMPATIBLE_SPIF_NOR_DT_H__

#include <driver.h>
#include <drivers/mtd/spif-nor.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int spif_nor_dt_read_config(
	spif_nor_t *nor, int node, sunxi_spif_t *spif)
{
	const dt2c_fdt32_t *frequency;
	const dt2c_fdt32_t *reg;
	spif_nor_t config = { 0 };
	uint32_t chip_select;
	uint32_t max_frequency;
	int parent;

	if (nor == NULL || spif == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SPI_NOR_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	parent = dt2c_fdt_parent_offset(DT2C_FDT_COMPILED_TREE, node);
	reg = syterkit_dt_cells(node, "reg", 1);
	frequency = syterkit_dt_cells(node, "spi-max-frequency", 1);
	if (parent < 0 || parent != spif->dt_node || reg == NULL || frequency == NULL)
		return DRIVER_ERROR_INVALID;

	chip_select = dt2c_fdt32_to_cpu(reg[0]);
	max_frequency = dt2c_fdt32_to_cpu(frequency[0]);
	if (chip_select > 3U || max_frequency == 0U || max_frequency > SUNXI_SPIF_MAX_FREQUENCY)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.chip_select = (uint8_t)chip_select;
	config.max_frequency = max_frequency;
	config.spif = spif;
	*nor = config;
	SYTERKIT_DT_TRACE_NODE("spif-nor", node);
	SYTERKIT_DT_TRACE("spif-nor config spif=%p chip_select=%u max_frequency=%u\n", (void *)nor->spif,
		nor->chip_select, nor->max_frequency);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int spif_nor_dt_read_alias(
	spif_nor_t *nor, const char *alias, sunxi_spif_t *spif)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SPI_NOR_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return spif_nor_dt_read_config(nor, node, spif);
}

#endif /* __DT_COMPATIBLE_SPIF_NOR_DT_H__ */
