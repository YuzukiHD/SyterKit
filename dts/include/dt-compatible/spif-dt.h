/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SPIF_DT_H__
#define __DT_COMPATIBLE_SPIF_DT_H__

#include <driver.h>
#include <drivers/spif/spif.h>
#include <dt-compatible/dt-common.h>
#include <dt-compatible/pinctrl-dt.h>

static inline __attribute__((always_inline)) uint32_t sunxi_spif_dt_optional_u32(
	int node, const char *name, uint32_t fallback)
{
	const dt2c_fdt32_t *cells = syterkit_dt_cells(node, name, 1);

	return cells != NULL ? dt2c_fdt32_to_cpu(cells[0]) : fallback;
}

static inline __attribute__((always_inline)) int sunxi_spif_dt_read_config(sunxi_spif_t *spif, int node)
{
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	sunxi_gpio_t gpio_controller;
	size_t pin_cell_count;
	uint32_t rx_width;
	uint32_t tx_width;

	if (spif == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_SPIF_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*spif = (sunxi_spif_t){ 0 };

	reg = syterkit_dt_cells(node, "reg", 2);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", 5);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	pins = syterkit_dt_pinctrl(node, &pin_cell_count, &gpio_controller);
	if (reg == NULL || module_clock == NULL || clock_gate == NULL || reset == NULL || pins == NULL ||
		pin_cell_count < 9U || pin_cell_count > 18U || pin_cell_count % 3U != 0U)
		return DRIVER_ERROR_INVALID;

	if (dt2c_fdt32_to_cpu(reg[0]) == 0U || sunxi_spif_dt_optional_u32(node, "allwinner,spif-id", 0U) > 3U ||
		dt2c_fdt32_to_cpu(module_clock[0]) == 0U || dt2c_fdt32_to_cpu(module_clock[3]) == 0U ||
		dt2c_fdt32_to_cpu(clock_gate[0]) == 0U || dt2c_fdt32_to_cpu(reset[0]) == 0U ||
		dt2c_fdt32_to_cpu(module_clock[1]) > 7U || dt2c_fdt32_to_cpu(module_clock[2]) >= 32U ||
		dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U || dt2c_fdt32_to_cpu(reset[1]) >= 32U ||
		!sunxi_gpio_dt_read_pin(&spif->gpio_cs, &gpio_controller, pins) ||
		!sunxi_gpio_dt_read_pin(&spif->gpio_sck, &gpio_controller, pins + 3) ||
		!sunxi_gpio_dt_read_pin(&spif->gpio_mosi, &gpio_controller, pins + 6))
		return DRIVER_ERROR_INVALID;

	if (pin_cell_count >= 12U && !sunxi_gpio_dt_read_pin(&spif->gpio_miso, &gpio_controller, pins + 9))
		return DRIVER_ERROR_INVALID;
	if (pin_cell_count >= 15U && !sunxi_gpio_dt_read_pin(&spif->gpio_wp, &gpio_controller, pins + 12))
		return DRIVER_ERROR_INVALID;
	if (pin_cell_count >= 18U && !sunxi_gpio_dt_read_pin(&spif->gpio_hold, &gpio_controller, pins + 15))
		return DRIVER_ERROR_INVALID;

	rx_width = sunxi_spif_dt_optional_u32(node, "spif-rx-bus-width", 1U);
	tx_width = sunxi_spif_dt_optional_u32(node, "spif-tx-bus-width", 1U);
	if ((rx_width != 1U && rx_width != 2U && rx_width != 4U && rx_width != 8U) ||
		(tx_width != 1U && tx_width != 4U && tx_width != 8U))
		return DRIVER_ERROR_INVALID;

	spif->dt_node = node;
	spif->base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	spif->id = (uint8_t)sunxi_spif_dt_optional_u32(node, "allwinner,spif-id", 0U);
	spif->bus_freq = sunxi_spif_dt_optional_u32(node, "clock-frequency", SUNXI_SPIF_MAX_FREQUENCY);
	spif->min_speed_hz = SUNXI_SPIF_MIN_FREQUENCY;
	spif->max_speed_hz = SUNXI_SPIF_MAX_FREQUENCY;
	if (spif->bus_freq < spif->min_speed_hz || spif->bus_freq > spif->max_speed_hz)
		return DRIVER_ERROR_INVALID;

	spif->clock_reg = (uintptr_t)dt2c_fdt32_to_cpu(module_clock[0]);
	spif->clock_source = dt2c_fdt32_to_cpu(module_clock[1]);
	spif->clock_n_offset = dt2c_fdt32_to_cpu(module_clock[2]);
	spif->clock_parent_hz = dt2c_fdt32_to_cpu(module_clock[3]);
	spif->clock_layout = sunxi_spif_dt_optional_u32(node, "allwinner,clock-layout", SUNXI_SPIF_CLOCK_LAYOUT_NM);
	if (spif->clock_layout > SUNXI_SPIF_CLOCK_LAYOUT_DIV2 ||
		(spif->clock_layout == SUNXI_SPIF_CLOCK_LAYOUT_NM && spif->clock_n_offset > 30U))
		return DRIVER_ERROR_INVALID;
	spif->clk.gate_reg_base = (uintptr_t)dt2c_fdt32_to_cpu(clock_gate[0]);
	spif->clk.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	spif->clk.rst_reg_base = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
	spif->clk.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	spif->clk.parent_clk = spif->clock_parent_hz;
	spif->mode = 0U;
	if (rx_width == 2U)
		spif->mode |= SPIF_RX_DUAL;
	else if (rx_width == 4U)
		spif->mode |= SPIF_RX_QUAD;
	else if (rx_width == 8U)
		spif->mode |= SPIF_RX_OCTAL;
	else
		spif->mode |= SPIF_RX_SLOW;
	if (tx_width == 4U)
		spif->mode |= SPIF_TX_QUAD;
	else if (tx_width == 8U)
		spif->mode |= SPIF_TX_OCTAL;
	else
		spif->mode |= SPIF_TX_BYTE;
	if (sunxi_spif_dt_optional_u32(node, "dtr-enable", 0U) != 0U)
		spif->mode |= SPIF_DTR_MODE;
	if (sunxi_spif_dt_optional_u32(node, "io-mode-enable", 0U) != 0U)
		spif->mode |= SPIF_IO_MODE;
	spif->sample_mode = SUNXI_SPIF_SAMPLE_DEFAULT;
	spif->sample_delay = SUNXI_SPIF_SAMPLE_DEFAULT;
	spif->initialized = 0U;
	SYTERKIT_DT_TRACE_NODE("spif", node);
	SYTERKIT_DT_TRACE("spif config base=%p clock=%p parent=%u rate=%u\n", (void *)spif->base,
		(void *)spif->clock_reg, spif->clock_parent_hz, spif->bus_freq);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_spif_dt_read_alias(sunxi_spif_t *spif, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_SPIF_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_spif_dt_read_config(spif, node);
}

#endif /* __DT_COMPATIBLE_SPIF_DT_H__ */
