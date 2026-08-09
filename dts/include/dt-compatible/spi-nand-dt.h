/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SPI_NAND_DT_H__
#define __DT_COMPATIBLE_SPI_NAND_DT_H__

#include <driver.h>
#include <drivers/mtd/spi-nand.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int
spi_nand_dt_read_config(spi_nand_t *nand, int node, sunxi_spi_t *spi) {
	const dt2c_fdt32_t *frequency;
	const dt2c_fdt32_t *reg;
	spi_nand_t config = {0};
	uint32_t chip_select;
	uint32_t max_frequency;
	int parent;

	if (nand == NULL || spi == NULL || node < 0 ||
	    !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SPI_NAND_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	parent = dt2c_fdt_parent_offset(DT2C_FDT_COMPILED_TREE, node);
	reg = syterkit_dt_cells(node, "reg", 1);
	frequency = syterkit_dt_cells(node, "spi-max-frequency", 1);
	if (parent < 0 || parent != spi->dt_node || reg == NULL ||
	    frequency == NULL)
		return DRIVER_ERROR_INVALID;

	chip_select = dt2c_fdt32_to_cpu(reg[0]);
	max_frequency = dt2c_fdt32_to_cpu(frequency[0]);
	if (chip_select > 3U || max_frequency == 0U ||
	    max_frequency > SPI_MAX_FREQUENCY)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.chip_select = (uint8_t) chip_select;
	config.max_frequency = max_frequency;
	config.spi = spi;
	*nand = config;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
spi_nand_dt_read_alias(spi_nand_t *nand, const char *alias,
		       sunxi_spi_t *spi) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SPI_NAND_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return spi_nand_dt_read_config(nand, node, spi);
}

#endif /* __DT_COMPATIBLE_SPI_NAND_DT_H__ */
