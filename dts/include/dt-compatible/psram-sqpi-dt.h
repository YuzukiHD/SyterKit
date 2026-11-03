/* SPDX-License-Identifier: MIT */

#ifndef SYTERKIT_PSRAM_SQPI_DT_H
#define SYTERKIT_PSRAM_SQPI_DT_H

#include <driver.h>
#include <drivers/clk/clk.h>
#include <drivers/gpio/gpio.h>
#include <drivers/psram/psram.h>
#include <dt-compatible/dt-common.h>
#include <dt-compatible/pinctrl-dt.h>

#define SUNXI_SQPI_PSRAM_COMPATIBLE "allwinner,sun252iw1-sqpi-psram"

static inline __attribute__((always_inline)) int sunxi_sqpi_psram_dt_read_config(sunxi_psram_t *psram, int node)
{
	const dt2c_fdt32_t *reg;

	if (!psram || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_SQPI_PSRAM_COMPATIBLE))
		return DRIVER_ERROR_INVALID;
	reg = syterkit_dt_cells(node, "reg", 4);
	if (!reg || !dt2c_fdt32_to_cpu(reg[0]) || dt2c_fdt32_to_cpu(reg[1]) < 0x1000U || !dt2c_fdt32_to_cpu(reg[2]) ||
		!dt2c_fdt32_to_cpu(reg[3]))
		return DRIVER_ERROR_INVALID;
	*psram = (sunxi_psram_t){ 0 };
	psram->dt_node = node;
	psram->memory_base = dt2c_fdt32_to_cpu(reg[2]);
	psram->memory_size = dt2c_fdt32_to_cpu(reg[3]);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_sqpi_psram_dt_read_alias(sunxi_psram_t *psram, const char *alias)
{
	if (!alias)
		return DRIVER_ERROR_INVALID;
	return sunxi_sqpi_psram_dt_read_config(psram, syterkit_dt_alias_node(alias, SUNXI_SQPI_PSRAM_COMPATIBLE));
}

static inline __attribute__((always_inline)) int sunxi_sqpi_psram_dt_read_clock(
	int node, uintptr_t *clock_reg, uint32_t *source, sunxi_clk_t *clk)
{
	const dt2c_fdt32_t *clock = syterkit_dt_cells(node, "allwinner,module-clock", 3);
	const dt2c_fdt32_t *gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	const dt2c_fdt32_t *reset = syterkit_dt_cells(node, "allwinner,reset", 2);

	if (!clock || !gate || !reset)
		return DRIVER_ERROR_INVALID;
	*clock_reg = dt2c_fdt32_to_cpu(clock[0]);
	*source = dt2c_fdt32_to_cpu(clock[1]);
	clk->parent_clk = dt2c_fdt32_to_cpu(clock[2]);
	clk->gate_reg_base = dt2c_fdt32_to_cpu(gate[0]);
	clk->gate_reg_offset = dt2c_fdt32_to_cpu(gate[1]);
	clk->rst_reg_base = dt2c_fdt32_to_cpu(reset[0]);
	clk->rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_sqpi_psram_dt_read_pins(int node, gpio_mux_t pins[6])
{
	sunxi_gpio_t gpio;
	const dt2c_fdt32_t *cells = syterkit_dt_pinctrl_cells(node, 18U, &gpio);

	if (!cells)
		return DRIVER_ERROR_INVALID;
	for (size_t i = 0; i < 6U; i++) {
		if (!sunxi_gpio_dt_read_pin(&pins[i], &gpio, cells + i * 3U))
			return DRIVER_ERROR_INVALID;
	}
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_sqpi_psram_dt_read_settings(
	int node, uintptr_t *base, uint32_t *frequency, size_t *capacity)
{
	const dt2c_fdt32_t *reg = syterkit_dt_cells(node, "reg", 4);
	const dt2c_fdt32_t *rate = syterkit_dt_cells(node, "clock-frequency", 1);
	const dt2c_fdt32_t *size = syterkit_dt_cells(node, "allwinner,capacity", 1);

	if (!reg || !rate || !size)
		return DRIVER_ERROR_INVALID;
	*base = dt2c_fdt32_to_cpu(reg[0]);
	*frequency = dt2c_fdt32_to_cpu(rate[0]);
	*capacity = dt2c_fdt32_to_cpu(size[0]);
	return DRIVER_OK;
}

#endif
