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

#include <sys-clic.h>
#include <sys-intc.h>

static irq_handler_t sunxi_int_handlers[CLIC_IRQ_NUM];
static irq_controller_t sunxi_clic_controller;

static int sunxi_plat_irq_init(void) {
	uint32_t i;
	irq_controller_t *ic;
	ic = &sunxi_clic_controller;
	ic->reg_base_addr = (SUNXI_RISCV_CLINT_BASE + 0x800000);
	ic->irq_cnt = 208;
	ic->parent_id = 0;
	ic->irq_id = 0;
	return 0;
}

static inline void sunxi_clic_set_irq_ctrl_bit(uint32_t reg_addr, uint8_t mask, int is_set) {
	uint8_t reg_data = readb(reg_addr);
	if (is_set)
		reg_data |= mask;
	else
		reg_data &= ~mask;
	writeb(reg_data, reg_addr);
}

static inline void sunxi_clic_set_enable(uint32_t reg_addr, int enabled) {
	sunxi_clic_set_irq_ctrl_bit(reg_addr, IE_BIT_MASK, enabled);
}

static inline void sunxi_clic_set_pending(uint32_t reg_addr, int pending) {
	sunxi_clic_set_irq_ctrl_bit(reg_addr, IP_BIT_MASK, pending);
}

static inline void sunxi_clic_set_vec_mode(uint32_t reg_addr, int vec_mode) {
	sunxi_clic_set_irq_ctrl_bit(reg_addr, HW_VECTOR_IRQ_BIT_MASK, vec_mode);
}

static inline void sunxi_clic_set_trigger_type(uint32_t reg_addr, irq_trigger_type_t type) {
	uint8_t reg_data, field_value;
	if (type == IRQ_TRIGGER_TYPE_LEVEL) {
		field_value = 0;
	} else if (type == IRQ_TRIGGER_TYPE_EDGE_RISING) {
		field_value = 1;
	} else if (type == IRQ_TRIGGER_TYPE_EDGE_FALLING) {
		field_value = 3;
	} else {
		return;
	}
	reg_data = readb(reg_addr);
	reg_data &= ~TRIGGER_TYPE_BIT_MASK;
	reg_data |= field_value << TRIGGER_TYPE_SHIFT;
	writeb(reg_data, reg_addr);
}

static int sunxi_clic_init(const struct irq_controller *ic) {
	uint32_t i, reg_data, irq_cnt, preemption_bits;
	uint32_t reg_addr;

	sunxi_plat_irq_init();
	reg_addr = ic->reg_base_addr + CLIC_INFO_REG_OFF;
	reg_data = readl(reg_addr);

	irq_cnt = (reg_data & IRQ_CNT_MASK) >> IRQ_CNT_SHIFT;
	printk_trace("CLIC: irq_cnt:%d ic->irq_cnt:%d\n", irq_cnt, ic->irq_cnt);
	if (ic->irq_cnt != irq_cnt)
		return -1;
	preemption_bits = (reg_data & CTRL_REG_BITS_MASK) >> CTRL_REG_BITS_SHIFT;
	preemption_bits <<= PREEMPTION_PRIORITY_BITS_SHIFT;
	preemption_bits &= PREEMPTION_PRIORITY_BITS_MASK;
	reg_addr = ic->reg_base_addr + CLIC_CFG_REG_OFF;

	writel(preemption_bits, reg_addr);
	for (i = 0; i < irq_cnt; i++) {
		//disable all interrupt
		reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(i);
		sunxi_clic_set_enable(reg_addr, 0);
		reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(i);
		sunxi_clic_set_vec_mode(reg_addr, 0);
		//clear pending
		reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(i);
		sunxi_clic_set_pending(reg_addr, 1);
	}
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(i);
	printk_trace("CLIC: addr 0x%x(0x%x)set_enable 0\n", reg_addr, readb(reg_addr));
	reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(i);
	printk_trace("CLIC: addr 0x%x(0x%x) set_vec 1\n", reg_addr, readb(reg_addr));
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(i);
	printk_trace("CLIC: addr 0x%x(0x%x) clear pending 1\n", reg_addr, readb(reg_addr));
	return 0;
}

static int sunxi_clic_irq_enable(const struct irq_controller *ic, uint32_t irq_id) {
	uint32_t reg_addr;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id);
	sunxi_clic_set_enable(reg_addr, 1);
	return 0;
}

static int sunxi_clic_irq_disable(const struct irq_controller *ic, uint32_t irq_id) {
	uint32_t reg_addr;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id);
	sunxi_clic_set_enable(reg_addr, 0);
	return 0;
}

static int sunxi_clic_irq_is_enabled(const struct irq_controller *ic, uint32_t irq_id) {
	if (readb((ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id))) & IE_BIT_MASK)
		return 1;
	return 0;
}

static int sunxi_clic_irq_is_pending(const struct irq_controller *ic, uint32_t irq_id) {
	if (readb((ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(irq_id))) & IP_BIT_MASK)
		return 1;
	return 0;
}

static int sunxi_clic_irq_set_pending(const struct irq_controller *ic, uint32_t irq_id, int pending) {
	uint32_t reg_addr;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(irq_id);
	sunxi_clic_set_pending(reg_addr, pending);
	return 0;
}

static int sunxi_clic_irq_set_trigger_type(const struct irq_controller *ic, uint32_t irq_id, irq_trigger_type_t type) {
	uint32_t reg_addr;
	if (type == IRQ_TRIGGER_TYPE_EDGE_BOTH)
		return -1;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(irq_id);
	sunxi_clic_set_trigger_type(reg_addr, type);
	return 0;
}

static void default_isr(void *data) {
	printk_debug("default_isr():  called from IRQ %d\n", (uint32_t) data);
	while (1)
		;
}

static void sunxi_clic_spi_handler(int irq_no) {
	if (sunxi_int_handlers[irq_no].func != default_isr) {
		sunxi_int_handlers[irq_no].func(sunxi_int_handlers[irq_no].data);
	}
}

void irq_free_handler(int irq) {
	if (irq >= CLIC_IRQ_NUM) {
		return;
	}
	sunxi_int_handlers[irq].data = NULL;
	sunxi_int_handlers[irq].func = default_isr;
}

int irq_enable(int irq_no) {
	if (irq_no >= CLIC_IRQ_NUM) {
		printf("CLIC: irq NO.(%d) > CLIC_IRQ_NUM(%d)\n", irq_no, CLIC_IRQ_NUM);
		return -1;
	}
	sunxi_clic_irq_enable(&sunxi_clic_controller, irq_no);
	return 0;
}

int irq_disable(int irq_no) {
	if (irq_no >= CLIC_IRQ_NUM) {
		printf("CLIC: irq NO.(%d) > CLIC_IRQ_NUM(%d) !!\n", irq_no, CLIC_IRQ_NUM);
		return -1;
	}
	sunxi_clic_irq_disable(&sunxi_clic_controller, irq_no);
	return 0;
}

void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data) {
	if (irq >= CLIC_IRQ_NUM || !handle_irq) {
		return;
	}
	sunxi_int_handlers[irq].data = data;
	sunxi_int_handlers[irq].func = handle_irq;
}

void do_irq(uint64_t cause) {
	uint32_t idnum = 0;
	csr_clear(mie, MIE_MSIE);

	do {
		idnum = (cause & 0xFFF);
		if (idnum != 0) {
			sunxi_clic_spi_handler(idnum);
			irq_enable(idnum);
			return;
		}
	} while (idnum != 0);

	csr_set(mie, MIE_MSIE);
	return;
}

int arch_interrupt_init(void) {
	sunxi_clic_init(&sunxi_clic_controller);
	csr_set(mstatus, MSTATUS_MIE);
	csr_set(mie, MIE_MEIE);
	return 0;
}

int arch_interrupt_exit(void) {
	csr_clear(mstatus, MSTATUS_MIE);
	csr_clear(mie, MIE_MEIE);
	/* reinit to clear pending */
	sunxi_clic_init(&sunxi_clic_controller);
	return 0;
}