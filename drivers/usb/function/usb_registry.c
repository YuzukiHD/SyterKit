/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>

#include <drivers/usb/function/usb_function.h>

/** @brief Compile-time list of USB functions available in this image. */
static const sunxi_usb_function_t *const sunxi_usb_functions[] = {
#if defined(CONFIG_DRIVER_USB_FUNCTION_WINUSB)
	&sunxi_usb_function_winusb,
#endif
#if defined(CONFIG_DRIVER_USB_FUNCTION_FEL)
	&sunxi_usb_function_fel,
#endif
	NULL,
};

/**
 * @brief Find a registered USB function by its numeric type.
 *
 * @param type The function type to find.
 * @return The matching function, or NULL when it is not registered.
 */
const sunxi_usb_function_t *sunxi_usb_function_lookup(uint32_t type)
{
	for (size_t index = 0; sunxi_usb_functions[index] != NULL; index++) {
		if (sunxi_usb_functions[index]->type == type)
			return sunxi_usb_functions[index];
	}

	return NULL;
}
