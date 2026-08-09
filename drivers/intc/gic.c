/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <driver.h>
#include <initcall.h>
#include <interrupt.h>
#include <io.h>
#include <log.h>

#include <drivers/intc/gic.h>
#include <dt-compatible/gic-dt.h>
#include <dt2c/driver.h>

enum {
	GICD_CTLR = 0x0000,
	GICD_TYPER = 0x0004,
	GICD_ISENABLER = 0x0100,
	GICD_ICENABLER = 0x0180,
	GICD_ICPENDR = 0x0280,
	GICD_ICACTIVER = 0x0380,
	GICD_IPRIORITYR = 0x0400,
	GICD_ITARGETSR = 0x0800,
	GICD_ICFGR = 0x0c00,
	GICC_CTLR = 0x0000,
	GICC_PMR = 0x0004,
	GICC_IAR = 0x000c,
	GICC_EOIR = 0x0010,
	GICC_DIR = 0x1000,
};

#define GIC_SPURIOUS_IRQ_MIN 1022U
#define GIC_IRQ_ID_MASK 0x3ffU

static irq_handler_t sunxi_int_handlers[SUNXI_GIC_MAX_IRQS];
static sunxi_gic_t sunxi_gic_controller;

static inline uintptr_t gicd_reg(const sunxi_gic_t *gic,
				 uint32_t offset) {
	return gic->distributor_base + offset;
}

static inline uintptr_t gicc_reg(const sunxi_gic_t *gic,
				 uint32_t offset) {
	return gic->cpu_interface_base + offset;
}

static bool sunxi_gic_config_valid(const sunxi_gic_t *gic) {
	return gic != NULL && gic->distributor_base != 0U &&
	       gic->distributor_size >= 0x1000U &&
	       gic->cpu_interface_base != 0U &&
	       gic->cpu_interface_size >= 0x1004U &&
	       gic->irq_count >= 32U &&
	       gic->irq_count <= SUNXI_GIC_MAX_IRQS;
}

static bool sunxi_gic_irq_valid(const sunxi_gic_t *gic, int irq) {
	return sunxi_gic_config_valid(gic) && gic->initialized && irq >= 0 &&
	       (uint32_t) irq < gic->irq_count;
}

static inline bool interrupts_are_enabled(void) {
	uint32_t cpsr;

	__asm__ __volatile__("mrs %0, cpsr" : "=r"(cpsr) : : "memory");
	return (cpsr & BIT(7)) == 0U;
}

static void default_isr(void *data) {
	printk_debug("default_isr(): called from IRQ %u\n",
		     (uint32_t) (uintptr_t) data);
	for (;;)
		;
}

static int gic_distributor_init(const sunxi_gic_t *gic) {
	uint32_t hardware_irq_count;
	uint32_t irq;

	writel(0U, gicd_reg(gic, GICD_CTLR));
	hardware_irq_count =
			((readl(gicd_reg(gic, GICD_TYPER)) & 0x1fU) + 1U) * 32U;
	if (hardware_irq_count > 1020U)
		hardware_irq_count = 1020U;
	if (hardware_irq_count < gic->irq_count) {
		printk_error("GIC: hardware has %u IRQs, devicetree requests %u\n",
			     hardware_irq_count, gic->irq_count);
		return DRIVER_ERROR_INVALID;
	}

	/* Configure SPIs as level-triggered and route them to CPU 0. */
	for (irq = 32U; irq < gic->irq_count; irq += 16U)
		writel(0U, gicd_reg(gic, GICD_ICFGR + (irq / 16U) * 4U));
	for (irq = 32U; irq < gic->irq_count; irq += 4U) {
		writel(0xa0a0a0a0U,
		       gicd_reg(gic, GICD_IPRIORITYR + irq));
		writel(0x01010101U,
		       gicd_reg(gic, GICD_ITARGETSR + irq));
	}
	for (irq = 32U; irq < gic->irq_count; irq += 32U) {
		writel(0xffffffffU,
		       gicd_reg(gic, GICD_ICENABLER + (irq / 32U) * 4U));
		writel(0xffffffffU,
		       gicd_reg(gic, GICD_ICACTIVER + (irq / 32U) * 4U));
	}

	writel(1U, gicd_reg(gic, GICD_CTLR));
	return DRIVER_OK;
}

static void gic_cpu_interface_init(const sunxi_gic_t *gic) {
	uint32_t irq;

	writel(0U, gicc_reg(gic, GICC_CTLR));
	writel(0xffff0000U, gicd_reg(gic, GICD_ICENABLER));
	writel(0x0000ffffU, gicd_reg(gic, GICD_ISENABLER));
	for (irq = 0U; irq < 32U; irq += 4U)
		writel(0xa0a0a0a0U,
		       gicd_reg(gic, GICD_IPRIORITYR + irq));
	writel(0xf0U, gicc_reg(gic, GICC_PMR));
	writel(1U, gicc_reg(gic, GICC_CTLR));
}

int sunxi_gic_init(sunxi_gic_t *gic) {
	int result;
	uint32_t irq;

	if (!sunxi_gic_config_valid(gic))
		return DRIVER_ERROR_INVALID;

	gic->initialized = false;
	for (irq = 0U; irq < gic->irq_count; irq++) {
		sunxi_int_handlers[irq].data = (void *) (uintptr_t) irq;
		sunxi_int_handlers[irq].func = default_isr;
	}

	result = gic_distributor_init(gic);
	if (result != DRIVER_OK)
		return result;
	gic_cpu_interface_init(gic);
	gic->initialized = true;
	return DRIVER_OK;
}

int sunxi_gic_exit(sunxi_gic_t *gic) {
	if (!sunxi_gic_config_valid(gic) || !gic->initialized)
		return DRIVER_ERROR_INVALID;

	writel(0U, gicc_reg(gic, GICC_CTLR));
	writel(0U, gicd_reg(gic, GICD_CTLR));
	gic->initialized = false;
	return DRIVER_OK;
}

static void gic_sgi_handler(uint32_t irq) {
	printk_debug("GIC: SGI IRQ %u\n", irq);
}

static void gic_ppi_handler(uint32_t irq) {
	printk_debug("GIC: PPI IRQ %u\n", irq);
}

static void gic_spi_handler(uint32_t irq) {
	irq_handler_t *handler = &sunxi_int_handlers[irq];

	if (handler->func != NULL && handler->func != default_isr)
		handler->func(handler->data);
}

static void gic_clear_pending(const sunxi_gic_t *gic, uint32_t irq) {
	writel(BIT(irq & 0x1fU),
	       gicd_reg(gic, GICD_ICPENDR + (irq / 32U) * 4U));
}

int arch_interrupt_init(void) {
	return sunxi_gic_init(&sunxi_gic_controller);
}

int arch_interrupt_exit(void) {
	return sunxi_gic_exit(&sunxi_gic_controller);
}

int sunxi_gic_cpu_interface_init(int cpu) {
	(void) cpu;
	if (!sunxi_gic_config_valid(&sunxi_gic_controller) ||
	    !sunxi_gic_controller.initialized)
		return DRIVER_ERROR_INVALID;

	gic_cpu_interface_init(&sunxi_gic_controller);
	return DRIVER_OK;
}

int sunxi_gic_cpu_interface_exit(void) {
	if (!sunxi_gic_config_valid(&sunxi_gic_controller) ||
	    !sunxi_gic_controller.initialized)
		return DRIVER_ERROR_INVALID;

	writel(0U, gicc_reg(&sunxi_gic_controller, GICC_CTLR));
	return DRIVER_OK;
}

void do_irq(struct arm_regs_t *regs) {
	uint32_t acknowledge;
	uint32_t irq;

	(void) regs;
	if (!sunxi_gic_controller.initialized)
		return;

	acknowledge = readl(gicc_reg(&sunxi_gic_controller, GICC_IAR));
	irq = acknowledge & GIC_IRQ_ID_MASK;
	if (irq >= GIC_SPURIOUS_IRQ_MIN)
		return;
	if (!sunxi_gic_irq_valid(&sunxi_gic_controller, (int) irq)) {
		printk_debug("GIC: invalid IRQ %u (source count %u)\n", irq,
			     sunxi_gic_controller.irq_count);
	} else if (irq < 16U) {
		gic_sgi_handler(irq);
	} else if (irq < 32U) {
		gic_ppi_handler(irq);
	} else {
		gic_spi_handler(irq);
	}

	writel(acknowledge, gicc_reg(&sunxi_gic_controller, GICC_EOIR));
	writel(acknowledge, gicc_reg(&sunxi_gic_controller, GICC_DIR));
	if (irq < sunxi_gic_controller.irq_count)
		gic_clear_pending(&sunxi_gic_controller, irq);
}

void irq_free_handler(int irq) {
	bool restore_interrupts;

	if (!sunxi_gic_irq_valid(&sunxi_gic_controller, irq))
		return;
	restore_interrupts = interrupts_are_enabled();
	if (restore_interrupts)
		arm32_interrupt_disable();
	sunxi_int_handlers[irq].data = (void *) (uintptr_t) irq;
	sunxi_int_handlers[irq].func = default_isr;
	if (restore_interrupts)
		arm32_interrupt_enable();
}

int irq_enable(int irq) {
	if (!sunxi_gic_irq_valid(&sunxi_gic_controller, irq)) {
		printk_error("GIC: invalid IRQ %d (source count %u)\n", irq,
			     sunxi_gic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	writel(BIT((uint32_t) irq & 0x1fU),
	       gicd_reg(&sunxi_gic_controller,
			 GICD_ISENABLER + ((uint32_t) irq / 32U) * 4U));
	return DRIVER_OK;
}

int irq_disable(int irq) {
	if (!sunxi_gic_irq_valid(&sunxi_gic_controller, irq)) {
		printk_error("GIC: invalid IRQ %d (source count %u)\n", irq,
			     sunxi_gic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	writel(BIT((uint32_t) irq & 0x1fU),
	       gicd_reg(&sunxi_gic_controller,
			 GICD_ICENABLER + ((uint32_t) irq / 32U) * 4U));
	return DRIVER_OK;
}

void irq_install_handler(int irq, interrupt_handler_t handler, void *data) {
	bool restore_interrupts;

	if (!sunxi_gic_irq_valid(&sunxi_gic_controller, irq) || handler == NULL)
		return;
	restore_interrupts = interrupts_are_enabled();
	if (restore_interrupts)
		arm32_interrupt_disable();
	sunxi_int_handlers[irq].data = data;
	sunxi_int_handlers[irq].func = handler;
	if (restore_interrupts)
		arm32_interrupt_enable();
}

static int sunxi_gic_initcall(void) {
	int result;

	result = sunxi_gic_dt_read_alias(&sunxi_gic_controller, "intc0");
	if (result != DRIVER_OK)
		return result;
	return arch_interrupt_init();
}

early_initcall(sunxi_gic_initcall);
DT2C_DRIVER_COMPAT("arm,gic-400");
