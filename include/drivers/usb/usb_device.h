/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file usb_device.h
 * @brief USB device (peripheral) mode configuration helpers.
 *
 * Provides low-level helpers to configure the USB controller for device mode
 * and to drive the device-side connection state.
 */

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__

#include <stdint.h>

/**
 * @brief Configure the USB controller for device-mode detection.
 */
void usb_device_config_detect_mode(uintptr_t base);

/**
 * @brief Switch the USB device connect state.
 */
void usb_device_connect_switch(uintptr_t base, uint32_t is_on);

#endif /* __USB_DEVICE_H__ */
