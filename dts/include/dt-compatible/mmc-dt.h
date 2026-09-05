/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_MMC_DT_H__
#define __DT_COMPATIBLE_MMC_DT_H__

#include <driver.h>
#include <drivers/mmc/sdhci.h>
#include <dt-compatible/pinctrl-dt.h>

#define SUNXI_MMC_COMPATIBLE "allwinner,sunxi-mmc"
#define SUNXI_MMC_MODULE_CLOCK_CELLS 4U
#define SUNXI_MMC_CARD_DETECT_CELLS 5U

static inline __attribute__((always_inline)) bool sunxi_sdhci_dt_clock_offsets_valid(uint32_t factor_n_offset, uint32_t factor_m_offset)
{
	uint32_t factor_n_mask;
	uint32_t factor_m_mask;
	const uint32_t fixed_mask = (0x3U << 24) | (1U << 31);

	if (factor_n_offset > 30U || factor_m_offset > 28U)
		return false;
	factor_n_mask = 0x3U << factor_n_offset;
	factor_m_mask = 0xfU << factor_m_offset;
	return (factor_n_mask & factor_m_mask) == 0U && (factor_n_mask & fixed_mask) == 0U && (factor_m_mask & fixed_mask) == 0U;
}

static inline __attribute__((always_inline)) const char *sunxi_sdhci_dt_name(int node)
{
	const char *name;
	int length;

	name = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "label", &length);
	if (name != NULL)
		return length > 1 && name[length - 1] == '\0' ? name : NULL;
	if (length != -DT2C_FDT_ERR_NOTFOUND)
		return NULL;
	name = dt2c_fdt_get_name(DT2C_FDT_COMPILED_TREE, node, &length);
	return name != NULL && length > 0 ? name : NULL;
}

static inline __attribute__((always_inline)) bool sunxi_sdhci_dt_dma_region(int node, uint32_t *address, uint32_t *size)
{
	const dt2c_fdt32_t *reg;
	uint32_t region_address;
	uint32_t region_size;
	int region;

	region = syterkit_dt_phandle_node(node, "memory-region", NULL);
	if (region < 0)
		return false;
	reg = syterkit_dt_cells(region, "reg", 2);
	if (reg == NULL)
		return false;
	region_address = dt2c_fdt32_to_cpu(reg[0]);
	region_size = dt2c_fdt32_to_cpu(reg[1]);
	if (region_address == 0U || region_address % __alignof__(sunxi_sdhci_desc_t) != 0U || region_size < sizeof(sunxi_sdhci_desc_t))
		return false;
	*address = region_address;
	*size = region_size;
	return true;
}

static inline __attribute__((always_inline)) bool sunxi_sdhci_dt_card_detect(int node, sunxi_sdhci_pinctrl_t *pinctrl)
{
	const dt2c_fdt32_t *cells;
	uint32_t level;
	int length;

	pinctrl->has_card_detect = false;
	cells = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,card-detect", &length);
	if (cells == NULL)
		return length == -DT2C_FDT_ERR_NOTFOUND;
	if (length != (int)(SUNXI_MMC_CARD_DETECT_CELLS * sizeof(*cells)))
		return false;
	level = dt2c_fdt32_to_cpu(cells[4]);
	if (level > GPIO_LEVEL_HIGH || !sunxi_gpio_dt_read_specifier(&pinctrl->gpio_cd, cells))
		return false;
	pinctrl->has_card_detect = true;
	pinctrl->cd_level = (uint8_t)level;
	return true;
}

static inline __attribute__((always_inline)) int sunxi_sdhci_dt_read_config(sunxi_sdhci_t *sdhci, int node)
{
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *clock_source_rates;
	const dt2c_fdt32_t *id_cells;
	const dt2c_fdt32_t *max_frequency;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *width_cells;
	const dt2c_fdt32_t *io_voltage;
	const void *non_removable;
	const void *sample_fifo_bypass;
	sunxi_gpio_t gpio_controller;
	sunxi_sdhci_t config = { 0 };
	uint32_t bus_width;
	uint32_t controller_id;
	uint32_t pin_count;
	int non_removable_length;
	int io_voltage_length;
	int sample_fifo_bypass_length;
	uint32_t io_voltage_uv = GPIO_IO_VOLTAGE_3V3;

	if (sdhci == NULL || node < 0 || !syterkit_dt_node_available(node) || dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_MMC_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	id_cells = syterkit_dt_cells(node, "allwinner,mmc-id", 1);
	width_cells = syterkit_dt_cells(node, "bus-width", 1);
	max_frequency = syterkit_dt_cells(node, "max-frequency", 1);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", SUNXI_MMC_MODULE_CLOCK_CELLS);
	clock_source_rates = syterkit_dt_cells(node, "allwinner,clock-source-rates", SUNXI_SDHCI_CLOCK_SOURCE_COUNT);
	config.name = sunxi_sdhci_dt_name(node);
	if (reg == NULL || id_cells == NULL || width_cells == NULL || max_frequency == NULL || clock_gate == NULL || reset == NULL || module_clock == NULL ||
	    clock_source_rates == NULL || config.name == NULL || !sunxi_sdhci_dt_dma_region(node, &config.dma_des_addr, &config.dma_des_size))
		return DRIVER_ERROR_INVALID;

	controller_id = dt2c_fdt32_to_cpu(id_cells[0]);
	bus_width = dt2c_fdt32_to_cpu(width_cells[0]);
	if (controller_id > MMC_CONTROLLER_2 || (bus_width != 1U && bus_width != 4U && bus_width != 8U) || dt2c_fdt32_to_cpu(reg[0]) == 0U || dt2c_fdt32_to_cpu(reg[1]) == 0U ||
	    dt2c_fdt32_to_cpu(max_frequency[0]) == 0U || dt2c_fdt32_to_cpu(clock_gate[0]) == 0U || dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U || dt2c_fdt32_to_cpu(reset[0]) == 0U ||
	    dt2c_fdt32_to_cpu(reset[1]) >= 32U || dt2c_fdt32_to_cpu(module_clock[0]) == 0U ||
	    !sunxi_sdhci_dt_clock_offsets_valid(dt2c_fdt32_to_cpu(module_clock[1]), dt2c_fdt32_to_cpu(module_clock[2])) ||
	    dt2c_fdt32_to_cpu(module_clock[3]) >= SUNXI_SDHCI_CLOCK_SOURCE_COUNT || dt2c_fdt32_to_cpu(clock_source_rates[0]) == 0U ||
	    dt2c_fdt32_to_cpu(clock_source_rates[dt2c_fdt32_to_cpu(module_clock[3])]) == 0U)
		return DRIVER_ERROR_INVALID;

	pin_count = bus_width + 2U + (bus_width == 8U ? 2U : 0U);
	pins = syterkit_dt_pinctrl_cells(node, (size_t)pin_count * 3U, &gpio_controller);
	if (pins == NULL || !syterkit_dt_pinctrl_gpio(pins, 0, &gpio_controller, &config.pinctrl.gpio_clk) ||
	    !syterkit_dt_pinctrl_gpio(pins, 3, &gpio_controller, &config.pinctrl.gpio_cmd) || !syterkit_dt_pinctrl_gpio(pins, 6, &gpio_controller, &config.pinctrl.gpio_d0) ||
	    (bus_width >= 4U &&
	     (!syterkit_dt_pinctrl_gpio(pins, 9, &gpio_controller, &config.pinctrl.gpio_d1) || !syterkit_dt_pinctrl_gpio(pins, 12, &gpio_controller, &config.pinctrl.gpio_d2) ||
	      !syterkit_dt_pinctrl_gpio(pins, 15, &gpio_controller, &config.pinctrl.gpio_d3))) ||
	    (bus_width == 8U &&
	     (!syterkit_dt_pinctrl_gpio(pins, 18, &gpio_controller, &config.pinctrl.gpio_d4) || !syterkit_dt_pinctrl_gpio(pins, 21, &gpio_controller, &config.pinctrl.gpio_d5) ||
	      !syterkit_dt_pinctrl_gpio(pins, 24, &gpio_controller, &config.pinctrl.gpio_d6) || !syterkit_dt_pinctrl_gpio(pins, 27, &gpio_controller, &config.pinctrl.gpio_d7) ||
	      !syterkit_dt_pinctrl_gpio(pins, 30, &gpio_controller, &config.pinctrl.gpio_ds) || !syterkit_dt_pinctrl_gpio(pins, 33, &gpio_controller, &config.pinctrl.gpio_rst))) ||
	    !sunxi_sdhci_dt_card_detect(node, &config.pinctrl))
		return DRIVER_ERROR_INVALID;

	non_removable = dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "non-removable", &non_removable_length);
	sample_fifo_bypass = dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,sample-fifo-bypass", &sample_fifo_bypass_length);
	io_voltage = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,io-voltage", &io_voltage_length);
	if ((non_removable != NULL && non_removable_length != 0) || (non_removable == NULL && non_removable_length != -DT2C_FDT_ERR_NOTFOUND) ||
	    (non_removable != NULL && config.pinctrl.has_card_detect) || (non_removable == NULL && bus_width == 8U))
		return DRIVER_ERROR_INVALID;
	if ((sample_fifo_bypass != NULL && sample_fifo_bypass_length != 0) ||
	    (sample_fifo_bypass == NULL && sample_fifo_bypass_length != -DT2C_FDT_ERR_NOTFOUND))
		return DRIVER_ERROR_INVALID;
	if (io_voltage != NULL && io_voltage_length != (int)sizeof(*io_voltage))
		return DRIVER_ERROR_INVALID;
	if (io_voltage == NULL && io_voltage_length != -DT2C_FDT_ERR_NOTFOUND)
		return DRIVER_ERROR_INVALID;
	if (io_voltage != NULL) {
		uint32_t value = dt2c_fdt32_to_cpu(io_voltage[0]);

		/* DT bindings normally use uV; accept mV too for pinconf-style trees. */
		if (value == 1800U || value == GPIO_IO_VOLTAGE_1V8)
			io_voltage_uv = GPIO_IO_VOLTAGE_1V8;
		else if (value == 3300U || value == GPIO_IO_VOLTAGE_3V3)
			io_voltage_uv = GPIO_IO_VOLTAGE_3V3;
		else
			return DRIVER_ERROR_INVALID;
	}

	config.dt_node = node;
	config.reg_base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	config.id = controller_id;
	if (bus_width == 1U)
		config.width = SMHC_WIDTH_1BIT;
	else if (bus_width == 4U)
		config.width = SMHC_WIDTH_4BIT;
	else
		config.width = SMHC_WIDTH_8BIT;
	config.max_clk = dt2c_fdt32_to_cpu(max_frequency[0]);
	config.sdhci_mmc_type = non_removable != NULL ? MMC_TYPE_EMMC : MMC_TYPE_SD;
	config.io_voltage_uv = io_voltage_uv;
	config.sample_fifo_bypass = sample_fifo_bypass != NULL;
	config.clk_ctrl.gate_reg_base = (uintptr_t)dt2c_fdt32_to_cpu(clock_gate[0]);
	config.clk_ctrl.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	config.clk_ctrl.rst_reg_base = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
	config.clk_ctrl.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	config.sdhci_clk.reg_base = (uintptr_t)dt2c_fdt32_to_cpu(module_clock[0]);
	config.sdhci_clk.reg_factor_n_offset = (uint8_t)dt2c_fdt32_to_cpu(module_clock[1]);
	config.sdhci_clk.reg_factor_m_offset = (uint8_t)dt2c_fdt32_to_cpu(module_clock[2]);
	config.sdhci_clk.default_clk_sel = (uint8_t)dt2c_fdt32_to_cpu(module_clock[3]);
	for (uint32_t source = 0U; source < SUNXI_SDHCI_CLOCK_SOURCE_COUNT; ++source)
		config.sdhci_clk.source_rates[source] = dt2c_fdt32_to_cpu(clock_source_rates[source]);
	*sdhci = config;
	SYTERKIT_DT_TRACE_NODE("mmc", node);
	SYTERKIT_DT_TRACE("mmc config name=%s base=%p id=%u width=%u max_clk=%u type=%u dma=%p/0x%x\n", sdhci->name, (void *)sdhci->reg_base, sdhci->id, sdhci->width,
			  sdhci->max_clk, sdhci->sdhci_mmc_type, (void *)(uintptr_t)sdhci->dma_des_addr, sdhci->dma_des_size);
	SYTERKIT_DT_TRACE("mmc clock gate=%p:%u reset=%p:%u module=%p source=%u rates=[%u,%u,%u,%u] n=%u m=%u\n", (void *)sdhci->clk_ctrl.gate_reg_base,
			  sdhci->clk_ctrl.gate_reg_offset, (void *)sdhci->clk_ctrl.rst_reg_base, sdhci->clk_ctrl.rst_reg_offset, (void *)sdhci->sdhci_clk.reg_base,
			  sdhci->sdhci_clk.default_clk_sel, sdhci->sdhci_clk.source_rates[0], sdhci->sdhci_clk.source_rates[1], sdhci->sdhci_clk.source_rates[2],
			  sdhci->sdhci_clk.source_rates[3], sdhci->sdhci_clk.reg_factor_n_offset, sdhci->sdhci_clk.reg_factor_m_offset);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_sdhci_dt_read_alias(sunxi_sdhci_t *sdhci, const char *alias)
{
	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_sdhci_dt_read_config(sdhci, syterkit_dt_alias_node(alias, SUNXI_MMC_COMPATIBLE));
}

#endif /* __DT_COMPATIBLE_MMC_DT_H__ */
