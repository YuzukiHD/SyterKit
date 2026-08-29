/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file usb_controller.c
 * @brief Allwinner USB controller mode and interrupt control.
 *
 * Programs the USB interface status/control register (ISCR) to force device
 * mode or valid VBUS, enable the ID and DPDM pull-ups, and manage the misc
 * USB interrupt enable and status registers.
 */

#include <stddef.h>
#include <stdint.h>

#include <io.h>

#include <drivers/usb/usb_controller.h>

/**
 * @brief Clear the change-detect bits in an ISCR value.
 *
 * @param[in] reg_val ISCR value to modify.
 * @return The value with the VBUS, ID, and DPDM change-detect bits cleared.
 */
static uint32_t usb_controller_clear_change_detect(uint32_t reg_val)
{
	reg_val &= ~(1U << USBC_BP_ISCR_VBUS_CHANGE_DETECT);
	reg_val &= ~(1U << USBC_BP_ISCR_ID_CHANGE_DETECT);
	reg_val &= ~(1U << USBC_BP_ISCR_DPDM_CHANGE_DETECT);
	return reg_val;
}

/**
 * @brief Force the USB controller into device mode.
 *
 * @param[in] base USB controller register base address.
 */
void usb_controller_force_device_mode(uintptr_t base)
{
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val &= ~(0x03U << USBC_BP_ISCR_FORCE_ID);
	reg_val |= 0x03U << USBC_BP_ISCR_FORCE_ID;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

/**
 * @brief Force the VBUS valid signal in the controller.
 *
 * @param[in] base USB controller register base address.
 */
void usb_controller_force_vbus_valid(uintptr_t base)
{
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val &= ~(0x03U << USBC_BP_ISCR_FORCE_VBUS_VALID);
	reg_val |= 0x03U << USBC_BP_ISCR_FORCE_VBUS_VALID;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

/**
 * @brief Enable the ID pin pull-up in the controller.
 *
 * @param[in] base USB controller register base address.
 */
void usb_controller_id_pull_enable(uintptr_t base)
{
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val |= 1U << USBC_BP_ISCR_ID_PULLUP_EN;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

/**
 * @brief Enable the DPDM pin pull-up in the controller.
 *
 * @param[in] base USB controller register base address.
 */
void usb_controller_dpdm_pull_enable(uintptr_t base)
{
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val |= 1U << USBC_BP_ISCR_DPDM_PULLUP_EN;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

/**
 * @brief Route the USB controller pins through the PIO path.
 *
 * @param[in] base USB controller register base address.
 */
void usb_controller_select_pio(uintptr_t base)
{
	if (base == 0U)
		return;

	writeb(0, USBC_REG_VEND0(base));
}

/**
 * @brief Disable all USB controller interrupts.
 *
 * @param[in] base USB controller register base address.
 */
void usb_controller_int_disable_all(uintptr_t base)
{
	if (base == 0U)
		return;

	writeb(0, USBC_REG_INTUSBE(base));
	writew(0, USBC_REG_INTTxE(base));
	writew(0, USBC_REG_INTRxE(base));
}

/**
 * @brief Enable USB misc interrupts selected by a mask.
 *
 * @param[in] base USB controller register base address.
 * @param[in] mask Misc interrupt enable bits to set.
 */
void usb_controller_int_enable_usb_misc_uint(uintptr_t base, uint32_t mask)
{
	uint8_t reg_val;

	if (base == 0U)
		return;

	reg_val = readb(USBC_REG_INTUSBE(base));
	writeb(reg_val | (uint8_t)mask, USBC_REG_INTUSBE(base));
}

/**
 * @brief Read the pending USB misc interrupt status.
 *
 * @param[in] base USB controller register base address.
 * @return The pending misc interrupt bits, or zero when @p base is invalid.
 */
uint32_t usb_controller_int_misc_pending(uintptr_t base)
{
	if (base == 0U)
		return 0;

	return readb(USBC_REG_INTUSB(base));
}

/**
 * @brief Clear pending USB misc interrupts selected by a mask.
 *
 * @param[in] base USB controller register base address.
 * @param[in] mask Misc interrupt bits to clear.
 */
void usb_controller_int_clear_misc_pending(uintptr_t base, uint32_t mask)
{
	if (base != 0U)
		writeb((uint8_t)mask, USBC_REG_INTUSB(base));
}
