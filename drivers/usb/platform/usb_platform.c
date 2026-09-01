/* SPDX-License-Identifier: GPL-2.0+ */

#include <driver.h>

#include <drivers/usb/platform/usb_platform.h>

int __attribute__((weak)) sunxi_usb_platform_init(const sunxi_usb_t *usb)
{
	return usb == NULL ? DRIVER_ERROR_INVALID : DRIVER_OK;
}

void __attribute__((weak)) sunxi_usb_platform_deinit(const sunxi_usb_t *usb)
{
	(void)usb;
}
