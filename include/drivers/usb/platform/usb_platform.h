/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_PLATFORM_USB_PLATFORM_H__
#define __DRIVERS_USB_PLATFORM_USB_PLATFORM_H__

#include <drivers/usb/usb_types.h>

int sunxi_usb_platform_init(const sunxi_usb_t *usb);
void sunxi_usb_platform_deinit(const sunxi_usb_t *usb);

#endif /* __DRIVERS_USB_PLATFORM_USB_PLATFORM_H__ */
