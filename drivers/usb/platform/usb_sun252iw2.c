/* SPDX-License-Identifier: GPL-2.0+ */
/* sun252iw2 reserves parts of the boot SRAM for the USB controller FIFO. */

#include <io.h>

#include <driver.h>
#include <dt-bindings/soc/sun252iw2.h>

#include <drivers/usb/platform/usb_platform.h>

#define SUN252IW2_SYSCTRL_SRAM_REMAP 0x004U
#define SUN252IW2_USB_SRAM_MASK (BIT(25) | BIT(27))

int sunxi_usb_platform_init(const sunxi_usb_t *usb)
{
	if (usb == NULL)
		return DRIVER_ERROR_INVALID;

	clrbits_le32(SUNXI_SYSCTRL_BASE + SUN252IW2_SYSCTRL_SRAM_REMAP, SUN252IW2_USB_SRAM_MASK);
	return DRIVER_OK;
}

void sunxi_usb_platform_deinit(const sunxi_usb_t *usb)
{
	(void)usb;
}
