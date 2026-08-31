/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PLIC_DT_H__
#define __DT_COMPATIBLE_PLIC_DT_H__

#include <driver.h>
#include <drivers/intc/plic.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int sunxi_plic_dt_read_config(sunxi_plic_t *plic, int node)
{
	const dt2c_fdt32_t *irq_count;
	const dt2c_fdt32_t *reg;
	sunxi_plic_t config = { 0 };

	if (plic == NULL || node < 0 || !syterkit_dt_node_available(node) || dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_PLIC_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	irq_count = syterkit_dt_cells(node, "riscv,num-sources", 1);
	if (reg == NULL || irq_count == NULL)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	config.size = dt2c_fdt32_to_cpu(reg[1]);
	config.irq_count = dt2c_fdt32_to_cpu(irq_count[0]);
	if (config.base == 0U || config.irq_count < 2U || config.irq_count > SUNXI_PLIC_MAX_IRQS || config.size < 0x200008U)
		return DRIVER_ERROR_INVALID;

	*plic = config;
	SYTERKIT_DT_TRACE_NODE("plic", node);
	SYTERKIT_DT_TRACE("plic config base=%p size=0x%lx irq_count=%u\n", (void *)plic->base, (unsigned long)plic->size, plic->irq_count);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_plic_dt_read_alias(sunxi_plic_t *plic, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_PLIC_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_plic_dt_read_config(plic, node);
}

#endif /* __DT_COMPATIBLE_PLIC_DT_H__ */
