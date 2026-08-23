/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DMA_DT_H__
#define __DT_COMPATIBLE_DMA_DT_H__

#include <driver.h>
#include <drivers/dma/dma.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int sunxi_dma_dt_read_config(sunxi_dma_t *dma, int node) {
	const dt2c_fdt32_t *bus_gate;
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	uintptr_t base;

	if (dma == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
									   SUNXI_DMA_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	bus_gate = syterkit_dt_cells(node, "allwinner,mbus-clock-gate", 2);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	if (reg == NULL || bus_gate == NULL || clock_gate == NULL ||
		reset == NULL)
		return DRIVER_ERROR_INVALID;

	base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	if (base == 0U || dt2c_fdt32_to_cpu(bus_gate[0]) == 0U ||
		dt2c_fdt32_to_cpu(clock_gate[0]) == 0U ||
		dt2c_fdt32_to_cpu(reset[0]) == 0U ||
		dt2c_fdt32_to_cpu(bus_gate[1]) >= 32U ||
		dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U ||
		dt2c_fdt32_to_cpu(reset[1]) >= 32U)
		return DRIVER_ERROR_INVALID;

	dma->dt_node = node;
	dma->dma_reg_base = base;
	dma->bus_clk.gate_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(bus_gate[0]);
	dma->bus_clk.gate_reg_offset = dt2c_fdt32_to_cpu(bus_gate[1]);
	dma->bus_clk.rst_reg_base = 0U;
	dma->bus_clk.rst_reg_offset = 0U;
	dma->bus_clk.parent_clk = 0U;
	dma->dma_clk.gate_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(clock_gate[0]);
	dma->dma_clk.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	dma->dma_clk.rst_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(reset[0]);
	dma->dma_clk.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	dma->dma_clk.parent_clk = 0U;
	dma->interrupt_count = 0;
	dma->initialized = false;
	SYTERKIT_DT_TRACE_NODE("dma", node);
	SYTERKIT_DT_TRACE("dma config base=%p mbus=%p:%u gate=%p:%u reset=%p:%u\n",
					  (void *) dma->dma_reg_base,
					  (void *) dma->bus_clk.gate_reg_base,
					  dma->bus_clk.gate_reg_offset,
					  (void *) dma->dma_clk.gate_reg_base,
					  dma->dma_clk.gate_reg_offset,
					  (void *) dma->dma_clk.rst_reg_base,
					  dma->dma_clk.rst_reg_offset);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_dma_dt_read_alias(sunxi_dma_t *dma, const char *alias) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_DMA_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_dma_dt_read_config(dma, node);
}

#endif /* __DT_COMPATIBLE_DMA_DT_H__ */
