/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_GIC_DT_H__
#define __DT_COMPATIBLE_GIC_DT_H__

#include <driver.h>
#include <drivers/intc/gic.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int
sunxi_gic_dt_read_config(sunxi_gic_t *gic, int node) {
	const dt2c_fdt32_t *irq_count;
	const dt2c_fdt32_t *reg;
	sunxi_gic_t config = {0};

	if (gic == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_GIC_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 4);
	irq_count = syterkit_dt_cells(node, "allwinner,irq-count", 1);
	if (reg == NULL || irq_count == NULL)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.distributor_base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	config.distributor_size = dt2c_fdt32_to_cpu(reg[1]);
	config.cpu_interface_base = (uintptr_t) dt2c_fdt32_to_cpu(reg[2]);
	config.cpu_interface_size = dt2c_fdt32_to_cpu(reg[3]);
	config.irq_count = dt2c_fdt32_to_cpu(irq_count[0]);
	if (config.distributor_base == 0U ||
	    config.distributor_size < 0x1000U ||
	    config.cpu_interface_base == 0U ||
	    config.cpu_interface_size < 0x1004U ||
	    config.irq_count < 32U ||
	    config.irq_count > SUNXI_GIC_MAX_IRQS)
		return DRIVER_ERROR_INVALID;

	*gic = config;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_gic_dt_read_alias(sunxi_gic_t *gic, const char *alias) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_GIC_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_gic_dt_read_config(gic, node);
}

#endif /* __DT_COMPATIBLE_GIC_DT_H__ */
