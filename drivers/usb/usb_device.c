/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file usb_device.c
 * @brief USB device-controller power and connect control.
 *
 * Configures the device power register for high-speed detection and controls
 * the soft-connect switch used to expose the device on the USB bus.
 */

#include <stddef.h>
#include <stdint.h>

#include <io.h>

#include <drivers/usb/usb_controller.h>
#include <drivers/usb/usb_device.h>

/**
 * @brief Configure the device controller for high-speed detection.
 *
 * @param[in] base USB controller register base address.
 */
void usb_device_config_detect_mode(uintptr_t base)
{
	uint8_t power;

	if (base == 0U)
		return;

	power = readb(USBC_REG_PCTL(base));
	power &= (uint8_t)~BIT(USBC_BP_POWER_D_ISO_UPDATE_EN);
	power |= BIT(USBC_BP_POWER_D_HIGH_SPEED_EN);
	writeb(power, USBC_REG_PCTL(base));
}

/**
 * @brief Set or clear the device soft-connect switch.
 *
 * @param[in] base USB controller register base address.
 * @param[in] is_on USBC_DEVICE_SWITCH_ON to connect, otherwise disconnect.
 */
void usb_device_connect_switch(uintptr_t base, uint32_t is_on)
{
	uint8_t power;

	if (base == 0U)
		return;

	power = readb(USBC_REG_PCTL(base));
	if (is_on == USBC_DEVICE_SWITCH_ON)
		power |= BIT(USBC_BP_POWER_D_SOFT_CONNECT);
	else
		power &= (uint8_t)~BIT(USBC_BP_POWER_D_SOFT_CONNECT);
	writeb(power, USBC_REG_PCTL(base));
}
