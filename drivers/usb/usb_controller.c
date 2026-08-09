/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>

#include <io.h>

#include <drivers/usb/usb_controller.h>

static uint32_t usb_controller_clear_change_detect(uint32_t reg_val) {
	reg_val &= ~(1U << USBC_BP_ISCR_VBUS_CHANGE_DETECT);
	reg_val &= ~(1U << USBC_BP_ISCR_ID_CHANGE_DETECT);
	reg_val &= ~(1U << USBC_BP_ISCR_DPDM_CHANGE_DETECT);
	return reg_val;
}

void usb_controller_force_device_mode(uintptr_t base) {
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val &= ~(0x03U << USBC_BP_ISCR_FORCE_ID);
	reg_val |= 0x03U << USBC_BP_ISCR_FORCE_ID;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

void usb_controller_force_vbus_valid(uintptr_t base) {
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val &= ~(0x03U << USBC_BP_ISCR_FORCE_VBUS_VALID);
	reg_val |= 0x03U << USBC_BP_ISCR_FORCE_VBUS_VALID;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

void usb_controller_id_pull_enable(uintptr_t base) {
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val |= 1U << USBC_BP_ISCR_ID_PULLUP_EN;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

void usb_controller_dpdm_pull_enable(uintptr_t base) {
	uint32_t reg_val;

	if (base == 0U)
		return;

	reg_val = read32(USBC_REG_ISCR(base));
	reg_val |= 1U << USBC_BP_ISCR_DPDM_PULLUP_EN;
	reg_val = usb_controller_clear_change_detect(reg_val);
	writel(reg_val, USBC_REG_ISCR(base));
}

void usb_controller_select_pio(uintptr_t base) {
	if (base == 0U)
		return;

	writeb(0, USBC_REG_VEND0(base));
}

void usb_controller_int_disable_all(uintptr_t base) {
	if (base == 0U)
		return;

	writeb(0, USBC_REG_INTUSBE(base));
	writew(0, USBC_REG_INTTxE(base));
	writew(0, USBC_REG_INTRxE(base));
}

void usb_controller_int_enable_usb_misc_uint(uintptr_t base, uint32_t mask) {
	uint8_t reg_val;

	if (base == 0U)
		return;

	reg_val = readb(USBC_REG_INTUSBE(base));
	writeb(reg_val | (uint8_t) mask,
	       USBC_REG_INTUSBE(base));
}

uint32_t usb_controller_int_misc_pending(uintptr_t base) {
	if (base == 0U)
		return 0;

	return readb(USBC_REG_INTUSB(base));
}

void usb_controller_int_clear_misc_pending(uintptr_t base, uint32_t mask) {
	if (base != 0U)
		writeb((uint8_t) mask, USBC_REG_INTUSB(base));
}
