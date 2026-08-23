/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <csr.h>
#include <driver.h>
#include <initcall.h>
#include <io.h>
#include <log.h>

#include <drivers/intc/clic.h>
#include <drivers/intc/intc.h>
#include <dt-compatible/clic-dt.h>
#include <dt2c/driver.h>

#include "clic-reg.h"

static sunxi_clic_t sunxi_clic_controller;

static bool sunxi_clic_config_valid(const sunxi_clic_t *clic) {
	return clic != NULL && clic->base != 0U && clic->irq_count != 0U &&
	       clic->irq_count <= SUNXI_CLIC_MAX_IRQS &&
	       clic->size >= 0x1000U + clic->irq_count * 4U;
}

static bool sunxi_clic_irq_valid(const sunxi_clic_t *clic, int irq) {
	return sunxi_clic_config_valid(clic) && clic->initialized && irq >= 0 &&
	       (uint32_t) irq < clic->irq_count;
}

static inline void sunxi_clic_set_irq_ctrl_bit(uintptr_t reg_addr,
					       uint8_t mask, bool set) {
	uint8_t reg_data = readb(reg_addr);

	if (set)
		reg_data |= mask;
	else
		reg_data &= (uint8_t) ~mask;
	writeb(reg_data, reg_addr);
}

static inline void sunxi_clic_set_enable(uintptr_t reg_addr, bool enabled) {
	sunxi_clic_set_irq_ctrl_bit(reg_addr, IE_BIT_MASK, enabled);
}

static inline void sunxi_clic_set_pending(uintptr_t reg_addr, bool pending) {
	sunxi_clic_set_irq_ctrl_bit(reg_addr, IP_BIT_MASK, pending);
}

static inline void sunxi_clic_set_vec_mode(uintptr_t reg_addr,
					   bool vector_mode) {
	sunxi_clic_set_irq_ctrl_bit(reg_addr, HW_VECTOR_IRQ_BIT_MASK,
				    vector_mode);
}

static int sunxi_clic_irq_enable(const sunxi_clic_t *clic, uint32_t irq) {
	if (!sunxi_clic_irq_valid(clic, (int) irq))
		return DRIVER_ERROR_INVALID;

	sunxi_clic_set_enable(clic->base + CLIC_INT_X_IE_REG_OFF(irq), true);
	return DRIVER_OK;
}

static int sunxi_clic_irq_disable(const sunxi_clic_t *clic, uint32_t irq) {
	if (!sunxi_clic_irq_valid(clic, (int) irq))
		return DRIVER_ERROR_INVALID;

	sunxi_clic_set_enable(clic->base + CLIC_INT_X_IE_REG_OFF(irq), false);
	return DRIVER_OK;
}

static void default_isr(void *data) {
	printk_debug("default_isr(): called from IRQ %u\n",
		     (uint32_t) (uintptr_t) data);
	for (;;)
		;
}

int sunxi_clic_init(sunxi_clic_t *clic) {
	uint32_t hardware_irq_count;
	uint32_t info;
	uint32_t preemption_bits;

	if (!sunxi_clic_config_valid(clic))
		return DRIVER_ERROR_INVALID;

	clic->initialized = false;
	info = readl(clic->base + CLIC_INFO_REG_OFF);
	hardware_irq_count = (info & IRQ_CNT_MASK) >> IRQ_CNT_SHIFT;
	printk_trace("CLIC: hardware sources %u, devicetree sources %u\n",
		     hardware_irq_count, clic->irq_count);
	if (hardware_irq_count != clic->irq_count)
		return DRIVER_ERROR_INVALID;

	preemption_bits = (info & CTRL_REG_BITS_MASK) >> CTRL_REG_BITS_SHIFT;
	preemption_bits <<= PREEMPTION_PRIORITY_BITS_SHIFT;
	preemption_bits &= PREEMPTION_PRIORITY_BITS_MASK;
	writel(preemption_bits, clic->base + CLIC_CFG_REG_OFF);

	for (uint32_t irq = 0; irq < clic->irq_count; irq++) {
		clic->handlers[irq].data = (void *) (uintptr_t) irq;
		clic->handlers[irq].func = default_isr;
		sunxi_clic_set_enable(
			clic->base + CLIC_INT_X_IE_REG_OFF(irq), false);
		sunxi_clic_set_vec_mode(
			clic->base + CLIC_INT_X_ATTR_REG_OFF(irq), false);

		/* clic pending is w1c regs */
		sunxi_clic_set_pending(
			clic->base + CLIC_INT_X_IP_REG_OFF(irq), true);
	}

	clic->initialized = true;
	return DRIVER_OK;
}

int sunxi_clic_exit(sunxi_clic_t *clic) {
	int result;

	if (clic == NULL || !clic->initialized)
		return DRIVER_ERROR_INVALID;

	result = sunxi_clic_init(clic);
	clic->initialized = false;
	return result;
}

static void sunxi_clic_irq_handler(int irq) {
	irq_handler_t *handler;

	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq))
		return;

	handler = &sunxi_clic_controller.handlers[irq];
	if (handler->func != NULL && handler->func != default_isr)
		handler->func(handler->data);
}

void irq_free_handler(int irq) {
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq))
		return;

	sunxi_clic_controller.handlers[irq].data =
		(void *) (uintptr_t) irq;
	sunxi_clic_controller.handlers[irq].func = default_isr;
}

int irq_enable(int irq) {
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq)) {
		printk_error("CLIC: invalid IRQ %d (source count %u)\n", irq,
			     sunxi_clic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	return sunxi_clic_irq_enable(&sunxi_clic_controller, (uint32_t) irq);
}

int irq_disable(int irq) {
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq)) {
		printk_error("CLIC: invalid IRQ %d (source count %u)\n", irq,
			     sunxi_clic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	return sunxi_clic_irq_disable(&sunxi_clic_controller, (uint32_t) irq);
}

void irq_install_handler(int irq, interrupt_handler_t handler, void *data) {
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq) ||
	    handler == NULL)
		return;

	sunxi_clic_controller.handlers[irq].data = data;
	sunxi_clic_controller.handlers[irq].func = handler;
}

void do_irq(uint64_t cause) {
	uint32_t irq = (uint32_t) (cause & 0xfffU);

	csr_clear(mie, MIE_MSIE);
	if (sunxi_clic_irq_valid(&sunxi_clic_controller, (int) irq)) {
		sunxi_clic_irq_handler((int) irq);
		(void) sunxi_clic_irq_enable(&sunxi_clic_controller, irq);
	}
	csr_set(mie, MIE_MSIE);
}

int arch_interrupt_init(void) {
	int result = sunxi_clic_init(&sunxi_clic_controller);

	if (result != DRIVER_OK)
		return result;
	csr_set(mstatus, MSTATUS_MIE);
	csr_set(mie, MIE_MEIE);
	return DRIVER_OK;
}

int arch_interrupt_exit(void) {
	csr_clear(mstatus, MSTATUS_MIE);
	csr_clear(mie, MIE_MEIE);
	return sunxi_clic_exit(&sunxi_clic_controller);
}

static int sunxi_clic_initcall(void) {
	int result;

	result = sunxi_clic_dt_read_alias(&sunxi_clic_controller, "intc0");
	if (result != DRIVER_OK)
		return result;
	return arch_interrupt_init();
}

early_initcall(sunxi_clic_initcall);
DT2C_DRIVER_COMPAT("thead,c900-clic");
