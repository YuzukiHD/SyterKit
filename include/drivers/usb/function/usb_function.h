/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_FUNCTION_USB_FUNCTION_H__
#define __DRIVERS_USB_FUNCTION_USB_FUNCTION_H__

#include <drivers/usb/usb_types.h>

#define SUNXI_USB_DEVICE_WINUSB 1U
#define SUNXI_USB_DEVICE_MASS	2U
#define SUNXI_USB_DEVICE_FEL	3U
#define SUNXI_USB_DEVICE_EFEX	SUNXI_USB_DEVICE_FEL

/** @brief Callbacks implemented by a USB device function. */
typedef struct sunxi_usb_function_ops {
	/** @brief Initialize function-specific state. */
	int (*state_init)(void);
	/** @brief Release function-specific state. */
	int (*state_exit)(void);
	/** @brief Reset function state after USB bus reset or disconnect. */
	void (*state_reset)(void);
	/** @brief Handle a standard USB control request. */
	int (*standard_req_op)(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer);
	/** @brief Handle a function-specific control request. */
	int (*nonstandard_req_op)(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status);
	/** @brief Run one iteration of the function data-path state machine. */
	int (*state_loop)(void *sunxi_udc);
	/** @brief Handle completion of a receive DMA transfer. */
	void (*dma_rx_isr)(void *p_arg);
	/** @brief Handle completion of a transmit DMA transfer. */
	void (*dma_tx_isr)(void *p_arg);
} sunxi_usb_function_ops_t;

/** @brief Compatibility name retained for out-of-tree USB functions. */
typedef sunxi_usb_function_ops_t sunxi_usb_setup_req_t;

/** @brief Descriptor and callback table for one USB device function. */
typedef struct sunxi_usb_function {
	/** @brief Numeric function type used by the compatibility registry. */
	uint32_t type;
	/** @brief Human-readable function name used in diagnostics. */
	const char *name;
	/** @brief Callback table implemented by the function. */
	const sunxi_usb_function_ops_t *ops;
} sunxi_usb_function_t;

#if defined(CONFIG_DRIVER_USB_FUNCTION_WINUSB)
/** @brief WinUSB device function implementation. */
extern const sunxi_usb_function_t sunxi_usb_function_winusb;
#endif

#if defined(CONFIG_DRIVER_USB_FUNCTION_FEL)
/** @brief FEL device function implementation. */
extern const sunxi_usb_function_t sunxi_usb_function_fel;
#endif

#endif /* __DRIVERS_USB_FUNCTION_USB_FUNCTION_H__ */
