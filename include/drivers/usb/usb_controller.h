/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __USB_CONTROLLER_H__
#define __USB_CONTROLLER_H__

#include <stdint.h>

#include "reg/reg-usb.h"

void usb_controller_force_device_mode(uintptr_t base);
void usb_controller_force_vbus_valid(uintptr_t base);
void usb_controller_id_pull_enable(uintptr_t base);
void usb_controller_dpdm_pull_enable(uintptr_t base);

void usb_controller_select_pio(uintptr_t base);
void usb_controller_int_disable_all(uintptr_t base);
void usb_controller_int_enable_usb_misc_uint(uintptr_t base, uint32_t mask);
uint32_t usb_controller_int_misc_pending(uintptr_t base);
void usb_controller_int_clear_misc_pending(uintptr_t base, uint32_t mask);

#endif /* __USB_CONTROLLER_H__ */
