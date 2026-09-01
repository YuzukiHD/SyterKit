/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>

#include <drivers/usb/function/usb_function.h>

static const sunxi_usb_function_t *const sunxi_usb_functions[] = {
#if defined(CONFIG_DRIVER_USB_FUNCTION_WINUSB)
	&sunxi_usb_function_winusb,
#endif
	NULL,
};

const sunxi_usb_function_t *sunxi_usb_function_lookup(uint32_t type)
{
	for (size_t index = 0; sunxi_usb_functions[index] != NULL; index++) {
		if (sunxi_usb_functions[index]->type == type)
			return sunxi_usb_functions[index];
	}

	return NULL;
}
