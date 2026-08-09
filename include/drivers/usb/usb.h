/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_H__
#define __USB_H__

#include <stdbool.h>
#include <stdint.h>

#define SUNXI_USB_COMPATIBLE "allwinner,sunxi-musb"
#define SUNXI_USB_MAX_CONTROLLERS 3U
typedef struct sunxi_usb sunxi_usb_t;

struct sunxi_usb {
	int dt_node;
	uint8_t id;
	uint8_t phy_clock_gate_offset;
	uint8_t phy_reset_offset;
	uint8_t clock_gate_offset;
	uint8_t reset_offset;
	uint32_t irq;
	uintptr_t base;
	uintptr_t phy_clock_reg_base;
	uintptr_t clock_gate_reg_base;

	volatile bool detected;
};

/**
 * @brief Initialize the USB controller
 *
 * This function initializes the controller and PHY for USB host detection,
 * then enables reset and disconnect interrupts.
 *
 * @param usb USB detector instance.
 * @return 0 on success, -1 on failure.
 */
int sunxi_usb_init(sunxi_usb_t *usb);

/**
 * @brief Handle the USB interrupt.
 *
 * This function detects a connected host from USB bus reset and disconnect
 * events.
 *
 * @param data USB detector instance passed by the interrupt framework.
 */
void sunxi_usb_irq(void *data);

/**
 * @brief Attach the USB device to the USB controller
 *
 * This function enables interrupts and waits until USB host activity is
 * detected.
 *
 * @param usb USB detector instance.
 */
void sunxi_usb_attach(sunxi_usb_t *usb);

#endif /* __USB_H__ */
