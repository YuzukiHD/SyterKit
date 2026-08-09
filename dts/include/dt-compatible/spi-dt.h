/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SPI_DT_H__
#define __DT_COMPATIBLE_SPI_DT_H__

#include <driver.h>
#include <drivers/spi.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/pinctrl-dt.h>

static inline __attribute__((always_inline)) int
sunxi_spi_dt_dma(int node, sunxi_dma_t *supplied_dma, sunxi_dma_t **dma,
		 uint8_t *rx_drq) {
	const dt2c_fdt32_t *cells;
	int dma_node;
	uint32_t drq;

	cells = syterkit_dt_cells(node, "dmas", 2);
	if (cells == NULL) {
		*dma = NULL;
		*rx_drq = 0U;
		return DRIVER_OK;
	}
	dma_node = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(cells[0]));
	drq = dt2c_fdt32_to_cpu(cells[1]);
	if (dma_node < 0 || !syterkit_dt_node_available(dma_node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, dma_node,
					   SUNXI_DMA_COMPATIBLE) != 0 ||
	    drq > 63U || supplied_dma == NULL ||
	    supplied_dma->dt_node != dma_node)
		return DRIVER_ERROR_INVALID;
	*dma = supplied_dma;
	*rx_drq = (uint8_t) drq;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_spi_dt_read_config(sunxi_spi_t *spi, int node,
			 sunxi_dma_t *supplied_dma) {
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *id_cells;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *value;
	sunxi_dma_t *dma;
	sunxi_spi_gpio_t gpio;
	uint32_t cdr_mode;
	uint32_t clock_rate;
	uint32_t parent_clock;
	uint32_t source;
	uint32_t id;
	uint8_t rx_drq;

	if (spi == NULL || node < 0 ||
	    !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_SPI_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	id_cells = syterkit_dt_cells(node, "allwinner,spi-id", 1);
	if (id_cells == NULL)
		return DRIVER_ERROR_INVALID;
	id = dt2c_fdt32_to_cpu(id_cells[0]);
	value = syterkit_dt_cells(node, "clock-frequency", 1);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", 5);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	pins = syterkit_dt_pinctrl_cells(node, 18);
	if (id >= SUNXI_SPI_CONTROLLER_MAX || reg == NULL || value == NULL ||
	    module_clock == NULL ||
	    clock_gate == NULL || reset == NULL || pins == NULL)
		return DRIVER_ERROR_INVALID;

	clock_rate = dt2c_fdt32_to_cpu(value[0]);
	source = dt2c_fdt32_to_cpu(module_clock[1]);
	parent_clock = dt2c_fdt32_to_cpu(module_clock[3]);
	cdr_mode = dt2c_fdt32_to_cpu(module_clock[4]);
	if (dt2c_fdt32_to_cpu(reg[0]) == 0U || clock_rate == 0U ||
	    source > 7U || dt2c_fdt32_to_cpu(module_clock[2]) >= 32U ||
	    cdr_mode > SPI_CDR_NONE ||
	    ((dt2c_fdt32_to_cpu(module_clock[0]) != 0U || parent_clock != 0U ||
	      dt2c_fdt32_to_cpu(clock_gate[0]) != 0U ||
	      dt2c_fdt32_to_cpu(reset[0]) != 0U) &&
	     (dt2c_fdt32_to_cpu(module_clock[0]) == 0U || parent_clock == 0U ||
	      dt2c_fdt32_to_cpu(clock_gate[0]) == 0U ||
	      dt2c_fdt32_to_cpu(reset[0]) == 0U)) ||
	    dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U ||
	    dt2c_fdt32_to_cpu(reset[1]) >= 32U ||
	    !syterkit_dt_gpio(pins, 0, &gpio.gpio_cs) ||
	    !syterkit_dt_gpio(pins, 3, &gpio.gpio_sck) ||
	    !syterkit_dt_gpio(pins, 6, &gpio.gpio_mosi) ||
	    !syterkit_dt_gpio(pins, 9, &gpio.gpio_miso) ||
	    !syterkit_dt_gpio(pins, 12, &gpio.gpio_wp) ||
	    !syterkit_dt_gpio(pins, 15, &gpio.gpio_hold) ||
	    sunxi_spi_dt_dma(node, supplied_dma, &dma, &rx_drq) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;

	spi->base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	spi->id = (uint8_t) id;
	spi->clk_rate = clock_rate;
	spi->gpio = gpio;
	spi->dma_handle = dma;
	spi->dma_rx_drq = rx_drq;
	spi->parent_clk_reg.gate_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(clock_gate[0]);
	spi->parent_clk_reg.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	spi->parent_clk_reg.rst_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(reset[0]);
	spi->parent_clk_reg.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	spi->parent_clk_reg.parent_clk = parent_clock;
	spi->spi_clk.spi_clock_cfg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(module_clock[0]);
	spi->spi_clk.spi_clock_source = source;
	spi->spi_clk.spi_clock_factor_n_offset =
			dt2c_fdt32_to_cpu(module_clock[2]);
	spi->spi_clk.spi_clock_freq = 0U;
	spi->spi_clk.cdr_mode = (spi_clk_cdr_mode_t) cdr_mode;
	spi->dma_handler = 0U;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_spi_dt_read_alias(sunxi_spi_t *spi, const char *alias,
			sunxi_dma_t *dma) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_SPI_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_spi_dt_read_config(spi, node, dma);
}

#endif /* __DT_COMPATIBLE_SPI_DT_H__ */
