/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <reg-ncat.h>

#include <sys-gic.h>

static irq_handler_t sunxi_int_handlers[GIC_IRQ_NUM];

/**
 * @brief Get interrupts state.
 * 
 * This inline function checks the state of interrupts. It reads the value of the CPSR register using assembly instructions 
 * and returns 1 if interrupts are open (CPSR bit 7 is cleared), otherwise it returns 0.
 * 
 * @return int Returns 1 if interrupts are open, otherwise returns 0.
 */
static inline int interrupts_is_open(void) {
	uint64_t temp = 0;
	__asm__ __volatile__("mrs %0, cpsr\n"
						 : "=r"(temp)
						 :
						 : "memory");
	return ((temp & 0x80) == 0) ? 1 : 0;
}

/**
 * @brief Set the target CPU for a specific SPI interrupt.
 * 
 * This inline function sets the target CPU for a specific SPI interrupt. It calculates the address of the target register 
 * based on the interrupt number, reads the current value of the register, modifies it to set the target CPU, and writes 
 * the modified value back to the register. The function assumes that the interrupt number is offset by 32 and that the 
 * maximum CPU ID is 15.
 * 
 * @param irq_no The interrupt number.
 * @param cpu_id The CPU ID of the target CPU.
 */
static inline void gic_spi_set_target(int irq_no, int cpu_id) {
	uint32_t reg_val, addr, offset;
	irq_no -= 32;
	/* dispatch the usb interrupt to CPU1 */
	addr = GIC_SPI_PROC_TARG(irq_no >> 2);
	reg_val = readl(addr);
	offset = 8 * (irq_no & 3);
	reg_val &= ~(0xff << offset);
	reg_val |= (((1 << cpu_id) & 0xf) << offset);
	writel(reg_val, addr);
	return;
}

/**
 * Initialize the GIC distributor controller
 *
 * @param void
 *
 * @return void
 */
static void gic_distributor_init(void) {
	uint32_t cpumask = 0x01010101;
	uint32_t gic_irqs;
	uint32_t i;

	writel(0x00, GIC_DIST_CON);

	/* check GIC hardware configutation */
	gic_irqs = ((readl(GIC_CON_TYPE) & 0x1f) + 1) * 32;
	if (gic_irqs > 1020) {
		gic_irqs = 1020;
	}

	if (gic_irqs < GIC_IRQ_NUM) {
		printk_error("GIC: parameter config error, only support %d irqs < %d(spec define)!!\n", gic_irqs, GIC_IRQ_NUM);
		return;
	}

	/* set trigger type to be level-triggered, active low */
	for (i = 0; i < GIC_IRQ_NUM; i += 16) { writel(0, GIC_IRQ_MOD_CFG(i >> 4)); }
	/* set priority */
	for (i = GIC_SRC_SPI(0); i < GIC_IRQ_NUM; i += 4) { writel(0xa0a0a0a0, GIC_SPI_PRIO((i - 32) >> 2)); }
	/* set processor target */
	for (i = 32; i < GIC_IRQ_NUM; i += 4) { writel(cpumask, GIC_SPI_PROC_TARG((i - 32) >> 2)); }
	/* disable all interrupts */
	for (i = 32; i < GIC_IRQ_NUM; i += 32) { writel(0xffffffff, GIC_CLR_EN(i >> 5)); }
	/* clear all interrupt active state */
	for (i = 32; i < GIC_IRQ_NUM; i += 32) { writel(0xffffffff, GIC_ACT_CLR(i >> 5)); }
	writel(1, GIC_DIST_CON);

	return;
}

static void gic_cpuif_init(void) {
	writel(0x00, GIC_CPU_IF_CTRL);
	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	*/
	writel(0xffff0000, GIC_CLR_EN(0));
	writel(0x0000ffff, GIC_SET_EN(0));
	/* Set priority on PPI and SGI interrupts */
	for (uint32_t i = 0; i < 16; i += 4) { writel(0xa0a0a0a0, GIC_SGI_PRIO(i >> 2)); }
	for (uint32_t i = 16; i < 32; i += 4) { writel(0xa0a0a0a0, GIC_PPI_PRIO((i - 16) >> 2)); }

	writel(0xf0, GIC_INT_PRIO_MASK);
	writel(0x01, GIC_CPU_IF_CTRL);
	return;
}

static void default_isr(void *data) {
	printk_debug("default_isr():  called from IRQ %d\n", (uint32_t) data);
	while (1)
		;
}

static void gic_sgi_handler(uint32_t irq_no) {
	printk_debug("GIC: SGI irq %d coming... \n", irq_no);
}

static void gic_ppi_handler(uint32_t irq_no) {
	printk_debug("GIC: PPI irq %d coming... \n", irq_no);
}

static void gic_spi_handler(uint32_t irq_no) {
	if (sunxi_int_handlers[irq_no].func != default_isr) {
		sunxi_int_handlers[irq_no].func(sunxi_int_handlers[irq_no].data);
	}
}

/**
 * @brief Clears the pending status of the specified IRQ in the GIC
 * 
 * @param irq_no IRQ number to clear pending status
 */
static void gic_clear_pending(uint32_t irq_no) {
	uint32_t offset = irq_no >> 5;
	uint32_t reg_val = (1 << (irq_no & 0x1f));
	writel(reg_val, GIC_PEND_CLR(offset));
	return;
}

int arch_interrupt_init(void) {
	for (int i = 0; i < GIC_IRQ_NUM; i++) sunxi_int_handlers[i].data = default_isr;
	gic_distributor_init();
	gic_cpuif_init();
	return 0;
}

int arch_interrupt_exit(void) {
	gic_distributor_init();
	gic_cpuif_init();
	return 0;
}

int sunxi_gic_cpu_interface_init(int cpu) {
	gic_cpuif_init();
	return 0;
}

int sunxi_gic_cpu_interface_exit(void) {
	writel(0, GIC_CPU_IF_CTRL);
	return 0;
}

void do_irq(struct arm_regs_t *regs) {
	uint32_t idnum = readl(GIC_INT_ACK_REG);

	if ((idnum == 1022) || (idnum == 1023)) {
		printk_error("GIC: spurious irq !!\n");
		return;
	}
	if (idnum >= GIC_IRQ_NUM) {
		printk_debug("GIC: irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", idnum, GIC_IRQ_NUM - 32);
		return;
	}
	if (idnum < 16)
		gic_sgi_handler(idnum);
	else if (idnum < 32)
		gic_ppi_handler(idnum);
	else
		gic_spi_handler(idnum);
	writel(idnum, GIC_END_INT_REG);
	writel(idnum, GIC_DEACT_INT_REG);
	gic_clear_pending(idnum);
	return;
}

void irq_free_handler(int irq) {
	arm32_interrupt_disable();
	if (irq >= GIC_IRQ_NUM) {
		arm32_interrupt_enable();
		return;
	}
	sunxi_int_handlers[irq].data = NULL;
	sunxi_int_handlers[irq].func = default_isr;
	arm32_interrupt_enable();
}

int irq_enable(int irq_no) {
	uint32_t reg_val;
	uint32_t offset;

	if (irq_no >= GIC_IRQ_NUM) {
		printk_error("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	offset = irq_no >> 5;
	reg_val = readl(GIC_SET_EN(offset));
	reg_val |= 1 << (irq_no & 0x1f);
	writel(reg_val, GIC_SET_EN(offset));
	return 0;
}

int irq_disable(int irq_no) {
	uint32_t reg_val;
	uint32_t offset;

	if (irq_no >= GIC_IRQ_NUM) {
		printk_error("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	gic_spi_set_target(irq_no, 0);
	offset = irq_no >> 5;
	reg_val = (1 << (irq_no & 0x1f));
	writel(reg_val, GIC_CLR_EN(offset));

	return 0;
}

void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data) {
	int flag = interrupts_is_open();
	//when irq_handler call this function , irq enable bit has already disabled in irq_mode,so don't need to enable I bit
	if (flag) {
		arm32_interrupt_disable();
		printk_error("IRQ OPEN\n");
	}

	if (irq >= GIC_IRQ_NUM || !handle_irq) {
		goto fail;
	}

	sunxi_int_handlers[irq].data = data;
	sunxi_int_handlers[irq].func = handle_irq;

fail:
	if (flag) {
		arm32_interrupt_enable();
		printk_error("IRQ OPEN\n");
	}
}