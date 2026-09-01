/* SPDX-License-Identifier: GPL-2.0+ */

#include <driver.h>

#include <drivers/usb/platform/usb_platform.h>

/**
 * @brief Run the default platform USB initialization hook.
 *
 * @param usb The controller description loaded from the devicetree.
 * @return Zero when the description is valid, otherwise a driver error.
 */
int __attribute__((weak)) sunxi_usb_platform_init(const sunxi_usb_t *usb)
{
	return usb == NULL ? DRIVER_ERROR_INVALID : DRIVER_OK;
}

/**
 * @brief Run the default platform USB deinitialization hook.
 *
 * @param usb The controller description loaded from the devicetree.
 */
void __attribute__((weak)) sunxi_usb_platform_deinit(const sunxi_usb_t *usb)
{
	(void)usb;
}
