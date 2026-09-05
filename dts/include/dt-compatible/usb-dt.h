/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_USB_DT_H__
#define __DT_COMPATIBLE_USB_DT_H__

#include <driver.h>
#include <drivers/usb/usb.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int sunxi_usb_dt_read_config(sunxi_usb_t *usb, int node)
{
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *id;
	const dt2c_fdt32_t *interrupt;
	const dt2c_fdt32_t *phy_clock;
	const dt2c_fdt32_t *phy_reset;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	uint32_t clock_gate_offset;
	uint32_t phy_clock_gate_offset;
	uint32_t phy_reset_offset;
	uint32_t reset_offset;
	uint32_t usb_id;
	sunxi_usb_t config = { 0 };

	if (usb == NULL || node < 0 || !syterkit_dt_node_available(node) || dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_USB_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	id = syterkit_dt_cells(node, "allwinner,usb-id", 1);
	interrupt = syterkit_dt_cells(node, "interrupts", 1);
	phy_clock = syterkit_dt_cells(node, "allwinner,phy-clock", 2);
	phy_reset = syterkit_dt_cells(node, "allwinner,phy-reset", 2);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	if (reg == NULL || id == NULL || phy_clock == NULL || phy_reset == NULL || clock_gate == NULL || reset == NULL)
		return DRIVER_ERROR_INVALID;

	usb_id = dt2c_fdt32_to_cpu(id[0]);
	phy_clock_gate_offset = dt2c_fdt32_to_cpu(phy_clock[1]);
	phy_reset_offset = dt2c_fdt32_to_cpu(phy_reset[1]);
	clock_gate_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	reset_offset = dt2c_fdt32_to_cpu(reset[1]);
	if (usb_id >= SUNXI_USB_MAX_CONTROLLERS || phy_clock_gate_offset >= 32U || phy_reset_offset >= 32U || clock_gate_offset >= 32U || reset_offset >= 32U)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	config.id = (uint8_t)usb_id;
	if (interrupt != NULL)
		config.irq = dt2c_fdt32_to_cpu(interrupt[0]);
	config.phy_clock_reg_base = (uintptr_t)dt2c_fdt32_to_cpu(phy_clock[0]);
	config.phy_clock_gate_offset = (uint8_t)phy_clock_gate_offset;
	config.phy_reset_offset = (uint8_t)phy_reset_offset;
	config.clock_gate_reg_base = (uintptr_t)dt2c_fdt32_to_cpu(clock_gate[0]);
	config.clock_gate_offset = (uint8_t)clock_gate_offset;
	config.reset_offset = (uint8_t)reset_offset;

	if (config.base == 0U || config.phy_clock_reg_base == 0U || config.clock_gate_reg_base == 0U ||
	    dt2c_fdt32_to_cpu(phy_reset[0]) != config.phy_clock_reg_base || dt2c_fdt32_to_cpu(reset[0]) != config.clock_gate_reg_base)
		return DRIVER_ERROR_INVALID;

	*usb = config;
	SYTERKIT_DT_TRACE_NODE("usb", node);
	SYTERKIT_DT_TRACE("usb config base=%p id=%u irq=%u phy=%p:%u/%u bus=%p:%u/%u\n", (void *)usb->base, usb->id, usb->irq, (void *)usb->phy_clock_reg_base,
			  usb->phy_clock_gate_offset, usb->phy_reset_offset, (void *)usb->clock_gate_reg_base, usb->clock_gate_offset, usb->reset_offset);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_usb_dt_read_alias(sunxi_usb_t *usb, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_USB_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_usb_dt_read_config(usb, node);
}

#endif /* __DT_COMPATIBLE_USB_DT_H__ */
