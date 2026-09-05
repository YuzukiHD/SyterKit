/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "plic: " fmt

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <barrier.h>
#include <csr.h>
#include <driver.h>
#include <io.h>
#include <log.h>

#include <drivers/intc/intc.h>
#include <drivers/intc/plic.h>
#include <dt-compatible/plic-dt.h>
#include <dt2c/driver.h>

#define PLIC_PRIORITY_OFFSET  0x000000U
#define PLIC_ENABLE_OFFSET    0x002000U
#define PLIC_CONTEXT_OFFSET   0x200000U
#define PLIC_THRESHOLD_OFFSET (PLIC_CONTEXT_OFFSET + 0x0U)
#define PLIC_CLAIM_OFFSET     (PLIC_CONTEXT_OFFSET + 0x4U)
#define PLIC_ENABLE_WORDS_MAX ((SUNXI_PLIC_MAX_IRQS + 31U) / 32U)
#define RISCV_MACHINE_EXTERNAL_INTERRUPT 11U

static sunxi_plic_t sunxi_plic_controller;

static bool sunxi_plic_config_valid(const sunxi_plic_t *plic)
{
	return plic != NULL && plic->base != 0U && plic->irq_count >= 2U && plic->irq_count <= SUNXI_PLIC_MAX_IRQS &&
	       plic->size >= PLIC_CLAIM_OFFSET + sizeof(uint32_t);
}

static bool sunxi_plic_irq_valid(const sunxi_plic_t *plic, int irq)
{
	return sunxi_plic_config_valid(plic) && plic->initialized && irq > 0 && (uint32_t)irq < plic->irq_count;
}

static uintptr_t sunxi_plic_priority_reg(const sunxi_plic_t *plic, uint32_t irq)
{
	return plic->base + PLIC_PRIORITY_OFFSET + irq * sizeof(uint32_t);
}

static uintptr_t sunxi_plic_enable_reg(const sunxi_plic_t *plic, uint32_t irq)
{
	return plic->base + PLIC_ENABLE_OFFSET + (irq / 32U) * sizeof(uint32_t);
}

static void default_isr(void *data)
{
	pr_warn("unhandled IRQ %u\n", (uint32_t)(uintptr_t)data);
}

int sunxi_plic_init(sunxi_plic_t *plic)
{
	uint32_t irq;
	uint32_t word;

	if (!sunxi_plic_config_valid(plic))
		return DRIVER_ERROR_INVALID;

	plic->initialized = false;

	for (word = 0U; word < PLIC_ENABLE_WORDS_MAX; ++word)
		writel(0U, plic->base + PLIC_ENABLE_OFFSET + word * sizeof(uint32_t));

	for (irq = 1U; irq < plic->irq_count; ++irq) {
		plic->handlers[irq].data = (void *)(uintptr_t)irq;
		plic->handlers[irq].func = default_isr;
		writel(0U, sunxi_plic_priority_reg(plic, irq));
	}

	writel(0U, plic->base + PLIC_THRESHOLD_OFFSET);
	plic->initialized = true;
	return DRIVER_OK;
}

int sunxi_plic_exit(sunxi_plic_t *plic)
{
	uint32_t word;

	if (plic == NULL || !plic->initialized)
		return DRIVER_ERROR_INVALID;

	for (word = 0U; word < PLIC_ENABLE_WORDS_MAX; ++word)
		writel(0U, plic->base + PLIC_ENABLE_OFFSET + word * sizeof(uint32_t));

	plic->initialized = false;
	return DRIVER_OK;
}

int irq_enable(int irq)
{
	uint32_t value;

	if (!sunxi_plic_irq_valid(&sunxi_plic_controller, irq)) {
		pr_err("invalid IRQ %d (source count %u)\n", irq, sunxi_plic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	writel(1U, sunxi_plic_priority_reg(&sunxi_plic_controller, (uint32_t)irq));
	value = readl(sunxi_plic_enable_reg(&sunxi_plic_controller, (uint32_t)irq));
	value |= BIT((uint32_t)irq % 32U);
	writel(value, sunxi_plic_enable_reg(&sunxi_plic_controller, (uint32_t)irq));

	pr_trace("enabled IRQ %d (source count %u)\n", irq, sunxi_plic_controller.irq_count);

	return DRIVER_OK;
}

int irq_disable(int irq)
{
	uint32_t value;

	if (!sunxi_plic_irq_valid(&sunxi_plic_controller, irq)) {
		pr_err("invalid IRQ %d (source count %u)\n", irq, sunxi_plic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	value = readl(sunxi_plic_enable_reg(&sunxi_plic_controller, (uint32_t)irq));
	value &= ~BIT((uint32_t)irq % 32U);
	writel(value, sunxi_plic_enable_reg(&sunxi_plic_controller, (uint32_t)irq));
	writel(0U, sunxi_plic_priority_reg(&sunxi_plic_controller, (uint32_t)irq));

	pr_trace("disabled IRQ %d (source count %u)\n", irq, sunxi_plic_controller.irq_count);

	return DRIVER_OK;
}

void irq_install_handler(int irq, interrupt_handler_t handler, void *data)
{
	if (!sunxi_plic_irq_valid(&sunxi_plic_controller, irq) || handler == NULL)
		return;

	sunxi_plic_controller.handlers[irq].data = data;
	sunxi_plic_controller.handlers[irq].func = handler;

	pr_trace("installed handler for IRQ %d handler %p\n", irq, (void *)handler);
}

void irq_free_handler(int irq)
{
	if (!sunxi_plic_irq_valid(&sunxi_plic_controller, irq))
		return;

	sunxi_plic_controller.handlers[irq].data = (void *)(uintptr_t)irq;
	sunxi_plic_controller.handlers[irq].func = default_isr;
	pr_trace("freed handler for IRQ %d\n", irq);
}

void sunxi_plic_handle_irq(void)
{
	uint32_t irq;

	if (!sunxi_plic_controller.initialized)
		return;

	for (;;) {
		irq = readl(sunxi_plic_controller.base + PLIC_CLAIM_OFFSET);

		if (irq == 0U)
			return;

		if (sunxi_plic_irq_valid(&sunxi_plic_controller, (int)irq))
			sunxi_plic_controller.handlers[irq].func(sunxi_plic_controller.handlers[irq].data);
		else
			pr_warn("spurious IRQ %u\n", irq);

		mb();
		writel(irq, sunxi_plic_controller.base + PLIC_CLAIM_OFFSET);
		mb();
	}
}

bool intc_handle_irq(unsigned long cause)
{
	unsigned long mie_state;

	if (cause != RISCV_MACHINE_EXTERNAL_INTERRUPT)
		return false;

	mie_state = csr_read(mie);
	csr_clear(mie, MIE_MEIE);
	sunxi_plic_handle_irq();
	if (mie_state & MIE_MEIE)
		csr_set(mie, MIE_MEIE);
	return true;
}

int arch_interrupt_init(void)
{
	int result = sunxi_plic_init(&sunxi_plic_controller);

	if (result != DRIVER_OK)
		return result;

	csr_set(mie, MIE_MEIE);
	csr_set(mstatus, MSTATUS_MIE);
	return DRIVER_OK;
}

int arch_interrupt_exit(void)
{
	csr_clear(mstatus, MSTATUS_MIE);
	csr_clear(mie, MIE_MEIE);
	return sunxi_plic_exit(&sunxi_plic_controller);
}

int sunxi_plic_startup(void)
{
	int result;

	result = sunxi_plic_dt_read_alias(&sunxi_plic_controller, "intc0");

	if (result != DRIVER_OK)
		return result;

	return arch_interrupt_init();
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-plic");
