/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__

#include <stdint.h>

void usb_device_config_detect_mode(uintptr_t base);
void usb_device_connect_switch(uintptr_t base, uint32_t is_on);

#endif /* __USB_DEVICE_H__ */
