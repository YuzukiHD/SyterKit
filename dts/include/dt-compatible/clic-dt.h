/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_CLIC_DT_H__
#define __DT_COMPATIBLE_CLIC_DT_H__

#include <driver.h>
#include <drivers/intc/clic.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int
sunxi_clic_dt_read_config(sunxi_clic_t *clic, int node) {
	const dt2c_fdt32_t *irq_count;
	const dt2c_fdt32_t *reg;
	uintptr_t base;
	size_t size;
	uint32_t sources;

	if (clic == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_CLIC_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	irq_count = syterkit_dt_cells(node, "riscv,num-sources", 1);
	if (reg == NULL || irq_count == NULL)
		return DRIVER_ERROR_INVALID;

	base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	size = dt2c_fdt32_to_cpu(reg[1]);
	sources = dt2c_fdt32_to_cpu(irq_count[0]);
	if (base == 0U || sources == 0U ||
	    sources > SUNXI_CLIC_MAX_IRQS ||
	    size < 0x1000U + sources * 4U)
		return DRIVER_ERROR_INVALID;

	clic->dt_node = node;
	clic->base = base;
	clic->size = size;
	clic->irq_count = sources;
	clic->initialized = false;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_clic_dt_read_alias(sunxi_clic_t *clic, const char *alias) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_CLIC_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_clic_dt_read_config(clic, node);
}

#endif /* __DT_COMPATIBLE_CLIC_DT_H__ */
