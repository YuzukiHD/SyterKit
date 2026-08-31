/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_PLATFORM_H__
#define __DRIVERS_USB_PLATFORM_H__

#include <drivers/usb/usb.h>

/**
 * Perform SoC-specific USB preparation before the controller is reset.
 *
 * Platforms may override this weak hook for SRAM routing, PHY tuning, or
 * other controller prerequisites not represented by the common registers.
 */
int sunxi_usb_platform_init(const sunxi_usb_t *usb);

/** Release SoC-specific USB resources after the controller is stopped. */
void sunxi_usb_platform_deinit(const sunxi_usb_t *usb);

#endif /* __DRIVERS_USB_PLATFORM_H__ */
