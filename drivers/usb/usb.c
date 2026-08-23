/* SPDX-License-Identifier:	GPL-2.0+ */
/* Based on https://github.com/allwinner-zh/bootloader */

#include <io.h>
#include <stdint.h>

#include <common.h>
#include <driver.h>
#include <interrupt.h>
#include <log.h>
#include <timer.h>

#include <drivers/intc/intc.h>

#include <drivers/usb/usb.h>
#include <drivers/usb/usb_controller.h>
#include <drivers/usb/usb_device.h>
#include <dt2c/driver.h>

#define SUNXI_USB_DETECT_TIMEOUT_US 5000000ULL

static void sunxi_usb_clock_deinit(const sunxi_usb_t *usb) {
	clrbits_le32(usb->clock_gate_reg_base, BIT(usb->reset_offset));
	mdelay(1);
	clrbits_le32(usb->clock_gate_reg_base, BIT(usb->clock_gate_offset));
	mdelay(1);
}

static void sunxi_usb_clock_init(const sunxi_usb_t *usb) {
	setbits_le32(usb->phy_clock_reg_base,
		     BIT(usb->phy_clock_gate_offset));
	mdelay(1);
	setbits_le32(usb->phy_clock_reg_base, BIT(usb->phy_reset_offset));
	mdelay(1);
	setbits_le32(usb->clock_gate_reg_base, BIT(usb->reset_offset));
	mdelay(1);
	setbits_le32(usb->clock_gate_reg_base, BIT(usb->clock_gate_offset));
	mdelay(1);
}

int sunxi_usb_init(sunxi_usb_t *usb) {
	uint32_t reg_val = 0;

	if (usb == NULL || usb->base == 0U || usb->irq == 0U ||
	    usb->phy_clock_reg_base == 0U || usb->clock_gate_reg_base == 0U ||
	    usb->id >= SUNXI_USB_MAX_CONTROLLERS)
		return -1;

	usb->detected = false;

	if (irq_disable(usb->irq) != DRIVER_OK)
		return -1;

	usb_device_connect_switch(usb->base, USBC_DEVICE_SWITCH_OFF);

	/* Close clock */
	sunxi_usb_clock_deinit(usb);

	/* open usb clock */
	sunxi_usb_clock_init(usb);

	/* disable OTG ID detect and set to device */
	usb_controller_force_device_mode(usb->base);

	/* Force VBUS to HIGH */
	usb_controller_force_vbus_valid(usb->base);

	/* disconnect usb */
	usb_device_connect_switch(usb->base, USBC_DEVICE_SWITCH_OFF);

	/* set pull up for dp dm and id */
	usb_controller_dpdm_pull_enable(usb->base);
	usb_controller_id_pull_enable(usb->base);

	usb_controller_select_pio(usb->base);

	/* Configure the PHY for high-speed device signalling. */
	usb_device_config_detect_mode(usb->base);

	usb_controller_int_disable_all(usb->base);
	usb_controller_int_clear_misc_pending(usb->base,
						USBC_INTUSB_RESET |
						USBC_INTUSB_DISCONNECT);

	/* A host-issued bus reset is the only positive detection event. */
	usb_controller_int_enable_usb_misc_uint(usb->base,
						USBC_INTUSB_RESET |
						USBC_INTUSB_DISCONNECT);

	/* set bit 1  ->  0 */
	reg_val = readl(usb->base + USBC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << 1);
	writel(reg_val, usb->base + USBC_REG_o_PHYCTL);

	reg_val = readl(usb->base + USBC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, usb->base + USBC_REG_o_PHYCTL);

	irq_install_handler(usb->irq, sunxi_usb_irq, usb);
	if (irq_enable(usb->irq) != DRIVER_OK) {
		irq_free_handler(usb->irq);
		usb_controller_int_disable_all(usb->base);
		usb_device_connect_switch(usb->base, USBC_DEVICE_SWITCH_OFF);
		sunxi_usb_clock_deinit(usb);
		return -1;
	}

	/* Soft-connect only after the reset detector is ready. */
	usb_device_connect_switch(usb->base, USBC_DEVICE_SWITCH_ON);

	return 0;
}

void sunxi_usb_irq(void *data) {
	sunxi_usb_t *usb = data;
	uint8_t misc_irq;
	uint8_t detect_irq;

	if (usb == NULL || usb->base == 0U)
		return;

	misc_irq = usb_controller_int_misc_pending(usb->base);
	detect_irq = misc_irq &
		     (USBC_INTUSB_RESET | USBC_INTUSB_DISCONNECT);
	if (detect_irq == 0U)
		return;

	usb_controller_int_clear_misc_pending(usb->base, detect_irq);
	if (detect_irq & USBC_INTUSB_DISCONNECT) {
		printk_trace("USB: IRQ: disconnect\n");
		usb->detected = false;
	}

	if (detect_irq & USBC_INTUSB_RESET) {
		printk_trace("USB: IRQ: reset\n");
		usb->detected = true;
	}
}

void sunxi_usb_attach(sunxi_usb_t *usb) {
	uint64_t deadline;

	if (usb == NULL)
		return;

	interrupt_enable();
	deadline = time_us() + SUNXI_USB_DETECT_TIMEOUT_US;

	while (!usb->detected && time_us() < deadline)
		;
	if (!usb->detected)
		printk_warning("USB: host detection timeout\n");
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-musb");
