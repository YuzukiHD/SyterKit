/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_FUNCTION_USB_FUNCTION_H__
#define __DRIVERS_USB_FUNCTION_USB_FUNCTION_H__

#include <drivers/usb/usb_types.h>

#define SUNXI_USB_DEVICE_WINUSB 1U
#define SUNXI_USB_DEVICE_MASS	2U
#define SUNXI_USB_DEVICE_FEL	3U
#define SUNXI_USB_DEVICE_EFEX	SUNXI_USB_DEVICE_FEL

typedef struct sunxi_usb_function_ops {
	int (*state_init)(void);
	int (*state_exit)(void);
	void (*state_reset)(void);
	int (*standard_req_op)(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer);
	int (*nonstandard_req_op)(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status);
	int (*state_loop)(void *sunxi_udc);
	void (*dma_rx_isr)(void *p_arg);
	void (*dma_tx_isr)(void *p_arg);
} sunxi_usb_function_ops_t;

/* Compatibility alias for out-of-tree USB functions. */
typedef sunxi_usb_function_ops_t sunxi_usb_setup_req_t;

typedef struct sunxi_usb_function {
	uint32_t type;
	const char *name;
	const sunxi_usb_function_ops_t *ops;
} sunxi_usb_function_t;

const sunxi_usb_function_t *sunxi_usb_function_lookup(uint32_t type);

#if defined(CONFIG_DRIVER_USB_FUNCTION_WINUSB)
extern const sunxi_usb_function_t sunxi_usb_function_winusb;
#endif
#if defined(CONFIG_DRIVER_USB_FUNCTION_FEL)
extern const sunxi_usb_function_t sunxi_usb_function_fel;
#endif

#endif /* __DRIVERS_USB_FUNCTION_USB_FUNCTION_H__ */
