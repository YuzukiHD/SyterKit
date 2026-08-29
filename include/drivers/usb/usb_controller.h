/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file usb_controller.h
 * @brief USB controller register-level helper functions.
 *
 * Provides low-level routines to force device mode, control the VBUS and
 * ID pull states, select the PIO mode and manage USB MISC interrupts.
 */

#ifndef __USB_CONTROLLER_H__
#define __USB_CONTROLLER_H__

#include <stdint.h>

#include "reg/reg-usb.h"

/**
 * @brief Force the USB controller into device (peripheral) mode.
 */
void usb_controller_force_device_mode(uintptr_t base);

/**
 * @brief Force the USB VBUS valid indication.
 */
void usb_controller_force_vbus_valid(uintptr_t base);

/**
 * @brief Enable the USB ID pin pull.
 */
void usb_controller_id_pull_enable(uintptr_t base);

/**
 * @brief Enable the USB DP/DM pin pulls.
 */
void usb_controller_dpdm_pull_enable(uintptr_t base);

/**
 * @brief Select PIO-based access to the USB controller.
 */
void usb_controller_select_pio(uintptr_t base);

/**
 * @brief Disable all USB controller interrupts.
 */
void usb_controller_int_disable_all(uintptr_t base);

/**
 * @brief Enable the USB MISC interrupt with the given mask.
 */
void usb_controller_int_enable_usb_misc_uint(uintptr_t base, uint32_t mask);

/**
 * @brief Read the pending USB MISC interrupt status.
 */
uint32_t usb_controller_int_misc_pending(uintptr_t base);

/**
 * @brief Clear pending USB MISC interrupt sources.
 */
void usb_controller_int_clear_misc_pending(uintptr_t base, uint32_t mask);

#endif /* __USB_CONTROLLER_H__ */
