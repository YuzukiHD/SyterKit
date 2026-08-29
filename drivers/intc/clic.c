/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file clic.c
 * @brief RISC-V Core-Local Interrupt Controller (CLIC) driver.
 *
 * Implements the CLIC programming model for T-Head RISC-V cores: register
 * init, per-IRQ enable/pending/vector control, and the arch-level interrupt
 * entry points used by the kernel interrupt framework.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <csr.h>
#include <driver.h>
#include <io.h>
#include <log.h>

#include <drivers/intc/clic.h>
#include <drivers/intc/intc.h>
#include <dt-compatible/clic-dt.h>
#include <dt2c/driver.h>

#include "clic-reg.h"

static sunxi_clic_t sunxi_clic_controller;

/**
 * @brief Check the CLIC controller configuration.
 *
 * @param[in] clic CLIC controller descriptor.
 * @return true when the base, IRQ count, and register window are valid.
 */
static bool sunxi_clic_config_valid(const sunxi_clic_t *clic)
{
	return clic != NULL && clic->base != 0U && clic->irq_count != 0U && clic->irq_count <= SUNXI_CLIC_MAX_IRQS && clic->size >= 0x1000U + clic->irq_count * 4U;
}

/**
 * @brief Check that an IRQ number is within the initialized controller range.
 *
 * @param[in] clic CLIC controller descriptor.
 * @param[in] irq IRQ number to validate.
 * @return true when the controller is valid and @p irq is in range.
 */
static bool sunxi_clic_irq_valid(const sunxi_clic_t *clic, int irq)
{
	return sunxi_clic_config_valid(clic) && clic->initialized && irq >= 0 && (uint32_t)irq < clic->irq_count;
}

/**
 * @brief Set or clear a single control bit in a CLIC IRQ register.
 *
 * @param[in] reg_addr Address of the control register to update.
 * @param[in] mask Bit mask identifying the control bit.
 * @param[in] set true to set the bit, false to clear it.
 */
static inline void sunxi_clic_set_irq_ctrl_bit(uintptr_t reg_addr, uint8_t mask, bool set)
{
	uint8_t reg_data = readb(reg_addr);

	if (set)
		reg_data |= mask;
	else
		reg_data &= (uint8_t)~mask;
	writeb(reg_data, reg_addr);
}

/**
 * @brief Enable or disable an IRQ through its enable register.
 *
 * @param[in] reg_addr Address of the IRQ enable register.
 * @param[in] enabled true to enable, false to disable.
 */
static inline void sunxi_clic_set_enable(uintptr_t reg_addr, bool enabled)
{
	sunxi_clic_set_irq_ctrl_bit(reg_addr, IE_BIT_MASK, enabled);
}

/**
 * @brief Set or clear the pending bit for an IRQ.
 *
 * @param[in] reg_addr Address of the IRQ pending register.
 * @param[in] pending true to mark pending, false to clear.
 */
static inline void sunxi_clic_set_pending(uintptr_t reg_addr, bool pending)
{
	sunxi_clic_set_irq_ctrl_bit(reg_addr, IP_BIT_MASK, pending);
}

/**
 * @brief Enable or disable vectored (hardware) interrupt handling.
 *
 * @param[in] reg_addr Address of the IRQ attribute register.
 * @param[in] vector_mode true to select vectored mode, false for regular mode.
 */
static inline void sunxi_clic_set_vec_mode(uintptr_t reg_addr, bool vector_mode)
{
	sunxi_clic_set_irq_ctrl_bit(reg_addr, HW_VECTOR_IRQ_BIT_MASK, vector_mode);
}

/**
 * @brief Enable a single CLIC interrupt source.
 *
 * @param[in] clic CLIC controller descriptor.
 * @param[in] irq IRQ number to enable.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID when @p irq is invalid.
 */
static int sunxi_clic_irq_enable(const sunxi_clic_t *clic, uint32_t irq)
{
	if (!sunxi_clic_irq_valid(clic, (int)irq))
		return DRIVER_ERROR_INVALID;

	sunxi_clic_set_enable(clic->base + CLIC_INT_X_IE_REG_OFF(irq), true);
	return DRIVER_OK;
}

/**
 * @brief Disable a single CLIC interrupt source.
 *
 * @param[in] clic CLIC controller descriptor.
 * @param[in] irq IRQ number to disable.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID when @p irq is invalid.
 */
static int sunxi_clic_irq_disable(const sunxi_clic_t *clic, uint32_t irq)
{
	if (!sunxi_clic_irq_valid(clic, (int)irq))
		return DRIVER_ERROR_INVALID;

	sunxi_clic_set_enable(clic->base + CLIC_INT_X_IE_REG_OFF(irq), false);
	return DRIVER_OK;
}

/**
 * @brief Default interrupt handler for uninstalled IRQ slots.
 *
 * Logs the triggering IRQ and spins forever so a stray interrupt is noticed.
 *
 * @param[in] data Opaque data carrying the IRQ number.
 */
static void default_isr(void *data)
{
	pr_debug("default_isr(): called from IRQ %u\n", (uint32_t)(uintptr_t)data);
	for (;;)
		;
}

/**
 * @brief Initialize the CLIC controller.
 *
 * Verifies the configuration, programs the preemption-priority bits, installs
 * the default handler for every source, and clears all pending interrupts.
 *
 * @param[in,out] clic CLIC controller descriptor.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID on bad configuration.
 */
int sunxi_clic_init(sunxi_clic_t *clic)
{
	uint32_t hardware_irq_count;
	uint32_t info;
	uint32_t preemption_bits;

	if (!sunxi_clic_config_valid(clic))
		return DRIVER_ERROR_INVALID;

	clic->initialized = false;
	info = readl(clic->base + CLIC_INFO_REG_OFF);
	hardware_irq_count = (info & IRQ_CNT_MASK) >> IRQ_CNT_SHIFT;
	pr_trace("CLIC: hardware sources %u, devicetree sources %u\n", hardware_irq_count, clic->irq_count);
	if (hardware_irq_count != clic->irq_count)
		return DRIVER_ERROR_INVALID;

	preemption_bits = (info & CTRL_REG_BITS_MASK) >> CTRL_REG_BITS_SHIFT;
	preemption_bits <<= PREEMPTION_PRIORITY_BITS_SHIFT;
	preemption_bits &= PREEMPTION_PRIORITY_BITS_MASK;
	writel(preemption_bits, clic->base + CLIC_CFG_REG_OFF);

	for (uint32_t irq = 0; irq < clic->irq_count; irq++) {
		clic->handlers[irq].data = (void *)(uintptr_t)irq;
		clic->handlers[irq].func = default_isr;
		sunxi_clic_set_enable(clic->base + CLIC_INT_X_IE_REG_OFF(irq), false);
		sunxi_clic_set_vec_mode(clic->base + CLIC_INT_X_ATTR_REG_OFF(irq), false);

		/* clic pending is w1c regs */
		sunxi_clic_set_pending(clic->base + CLIC_INT_X_IP_REG_OFF(irq), true);
	}

	clic->initialized = true;
	return DRIVER_OK;
}

/**
 * @brief Tear down the CLIC controller.
 *
 * Re-runs initialization to restore the default handler state, then marks the
 * controller as uninitialized.
 *
 * @param[in,out] clic CLIC controller descriptor.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID when not initialized.
 */
int sunxi_clic_exit(sunxi_clic_t *clic)
{
	int result;

	if (clic == NULL || !clic->initialized)
		return DRIVER_ERROR_INVALID;

	result = sunxi_clic_init(clic);
	clic->initialized = false;
	return result;
}

/**
 * @brief Dispatch a single IRQ to its installed handler.
 *
 * @param[in] irq IRQ number to dispatch.
 */
static void sunxi_clic_irq_handler(int irq)
{
	irq_handler_t *handler;

	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq))
		return;

	handler = &sunxi_clic_controller.handlers[irq];
	if (handler->func != NULL && handler->func != default_isr)
		handler->func(handler->data);
}

/**
 * @brief Uninstall an interrupt handler, restoring the default handler.
 *
 * @param[in] irq IRQ number whose handler should be freed.
 */
void irq_free_handler(int irq)
{
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq))
		return;

	sunxi_clic_controller.handlers[irq].data = (void *)(uintptr_t)irq;
	sunxi_clic_controller.handlers[irq].func = default_isr;
}

/**
 * @brief Enable an interrupt at the CLIC.
 *
 * @param[in] irq IRQ number to enable.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID for an out-of-range IRQ.
 */
int irq_enable(int irq)
{
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq)) {
		pr_err("CLIC: invalid IRQ %d (source count %u)\n", irq, sunxi_clic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	return sunxi_clic_irq_enable(&sunxi_clic_controller, (uint32_t)irq);
}

/**
 * @brief Disable an interrupt at the CLIC.
 *
 * @param[in] irq IRQ number to disable.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID for an out-of-range IRQ.
 */
int irq_disable(int irq)
{
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq)) {
		pr_err("CLIC: invalid IRQ %d (source count %u)\n", irq, sunxi_clic_controller.irq_count);
		return DRIVER_ERROR_INVALID;
	}

	return sunxi_clic_irq_disable(&sunxi_clic_controller, (uint32_t)irq);
}

/**
 * @brief Install an interrupt handler for an IRQ.
 *
 * @param[in] irq IRQ number to install the handler for.
 * @param[in] handler Handler function to invoke.
 * @param[in] data Opaque argument passed to @p handler.
 */
void irq_install_handler(int irq, interrupt_handler_t handler, void *data)
{
	if (!sunxi_clic_irq_valid(&sunxi_clic_controller, irq) || handler == NULL)
		return;

	sunxi_clic_controller.handlers[irq].data = data;
	sunxi_clic_controller.handlers[irq].func = handler;
}

/**
 * @brief CLIC interrupt entry point.
 *
 * Extracts the IRQ number from the machine cause register, dispatches it, and
 * re-enables machine interrupts.
 *
 * @param[in] cause Value of the machine cause register at interrupt entry.
 */
void do_irq(uint64_t cause)
{
	uint32_t irq = (uint32_t)(cause & 0xfffU);

	csr_clear(mie, MIE_MSIE);
	if (sunxi_clic_irq_valid(&sunxi_clic_controller, (int)irq)) {
		sunxi_clic_irq_handler((int)irq);
		(void)sunxi_clic_irq_enable(&sunxi_clic_controller, irq);
	}
	csr_set(mie, MIE_MSIE);
}

/**
 * @brief Initialize the arch interrupt subsystem.
 *
 * @return DRIVER_OK on success, otherwise the CLIC init error.
 */
int arch_interrupt_init(void)
{
	int result = sunxi_clic_init(&sunxi_clic_controller);

	if (result != DRIVER_OK)
		return result;
	csr_set(mstatus, MSTATUS_MIE);
	csr_set(mie, MIE_MEIE);
	return DRIVER_OK;
}

/**
 * @brief Tear down the arch interrupt subsystem.
 *
 * @return DRIVER_OK on success, otherwise the CLIC exit error.
 */
int arch_interrupt_exit(void)
{
	csr_clear(mstatus, MSTATUS_MIE);
	csr_clear(mie, MIE_MEIE);
	return sunxi_clic_exit(&sunxi_clic_controller);
}

/**
 * @brief CLIC startup entry: read the DT alias and initialize the controller.
 *
 * @return DRIVER_OK on success, otherwise the DT read or init error.
 */
int sunxi_clic_startup(void)
{
	int result;

	result = sunxi_clic_dt_read_alias(&sunxi_clic_controller, "intc0");
	if (result != DRIVER_OK)
		return result;
	return arch_interrupt_init();
}
DT2C_DRIVER_COMPAT("thead,c900-clic");
