/* SPDX-License-Identifier: GPL-2.0+ */
/* sun252iw2 reserves parts of the boot SRAM for the USB controller FIFO. */

#include <io.h>

#include <driver.h>
#include <dt-bindings/soc/sun252iw2.h>

#include <drivers/usb/platform/usb_platform.h>

#define SUN252IW2_SYSCTRL_SRAM_REMAP 0x004U
#define SUN252IW2_USB_SRAM_MASK	     (BIT(25) | BIT(27))

static uint32_t saved_sram_mapping;
static int sram_acquired;

/**
 * @brief Reserve the sun252iw2 SRAM regions used by the USB FIFO.
 *
 * @param usb The controller description loaded from the devicetree.
 * @return Zero on success, or a driver error when the description is NULL.
 */
int sunxi_usb_platform_init(const sunxi_usb_t *usb)
{
	if (usb == NULL)
		return DRIVER_ERROR_INVALID;

	if (!sram_acquired) {
		saved_sram_mapping = readl(SUNXI_SYSCTRL_BASE + SUN252IW2_SYSCTRL_SRAM_REMAP) &
			SUN252IW2_USB_SRAM_MASK;
		sram_acquired = 1;
	}
	clrbits_le32(SUNXI_SYSCTRL_BASE + SUN252IW2_SYSCTRL_SRAM_REMAP, SUN252IW2_USB_SRAM_MASK);
	return DRIVER_OK;
}

/**
 * @brief Release platform-specific USB resources on sun252iw2.
 *
 * @param usb The controller description loaded from the devicetree.
 */
void sunxi_usb_platform_deinit(const sunxi_usb_t *usb)
{
	if (usb == NULL || !sram_acquired)
		return;

	/* Restore only the banks acquired here, after the controller is stopped.
	 * This restores the caller's mapping, not an assumed CPU mapping. */
	clrsetbits_le32(SUNXI_SYSCTRL_BASE + SUN252IW2_SYSCTRL_SRAM_REMAP,
		SUN252IW2_USB_SRAM_MASK, saved_sram_mapping);
	sram_acquired = 0;
}
