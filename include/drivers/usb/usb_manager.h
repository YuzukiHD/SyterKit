/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_MANAGER_H__
#define __DRIVERS_USB_MANAGER_H__

#include <drivers/usb/usb_types.h>
#include <drivers/usb/function/usb_function.h>

/** @brief Runtime state for the USB device controller. */
typedef struct sunxi_udc {
	uintptr_t usbc_hd;
	uint32_t address;
	uint32_t speed;
	uint32_t bulk_ep_max;
	uint32_t fifo_size;
	uint32_t bulk_in_addr;
	uint32_t bulk_out_addr;
	uint32_t dma_send_channal;
	uint32_t dma_recv_channal;
	uint32_t ep0_stage;
	struct usb_device_request standard_reg;
} sunxi_udc_t;

/** @brief Buffers and transfer state shared with an active USB function. */
typedef struct sunxi_ubuf {
	uint8_t *rx_base_buffer;
	uint8_t *rx_req_buffer;
	uint32_t rx_buffer_size;
	uint32_t rx_req_length;
	uint32_t rx_ready_for_data;
	uint32_t request_size;
} sunxi_ubuf_t;

#define SUNXI_USB_REQ_SUCCESSED 0
#define SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED (-1)
#define SUNXI_USB_REQ_UNKNOWN_COMMAND (-2)
#define SUNXI_USB_REQ_UNMATCHED_COMMAND (-3)
#define SUNXI_USB_REQ_DATA_HUNGRY (-4)
#define SUNXI_USB_REQ_OP_ERR (-5)

/**
 * @brief Attach a concrete USB function implementation.
 *
 * Passing the function object directly keeps unused function implementations
 * eligible for linker garbage collection.
 *
 * @param function The function implementation to activate.
 */
void sunxi_usb_attach_function(const sunxi_usb_function_t *function);

/**
 * @brief Attach a USB function selected by its legacy numeric type.
 *
 * @param device_type The legacy USB function type.
 */
void sunxi_usb_attach_module(uint32_t device_type);

/**
 * @brief Initialize the USB controller and device state.
 *
 * @return Zero on success, or a negative error code on failure.
 */
int sunxi_usb_init(void);

/**
 * @brief Print USB controller and endpoint registers for diagnostics.
 *
 * @param usbc_base The controller register base address.
 * @param ep_index The endpoint index to select while dumping registers.
 */
void sunxi_usb_dump(uint32_t usbc_base, uint32_t ep_index);

/** @brief Reset the control and bulk endpoint state. */
void sunxi_usb_ep_reset(void);

/**
 * @brief Handle a pending USB controller interrupt.
 *
 * @param data Interrupt callback context, unused by the controller.
 */
void sunxi_usb_irq(void *data);

/** @brief Run the USB function loop until the caller stops it. */
void sunxi_usb_attach(void);

/**
 * @brief Run one iteration of the active USB function loop.
 *
 * @return The active function's loop result, or a negative error code if no
 * function is attached.
 */
int sunxi_usb_extern_loop(void);

/** @brief Reinitialize the configured bulk endpoints. */
void sunxi_usb_bulk_ep_reset(void);

/**
 * @brief Start a DMA receive transfer on the bulk OUT endpoint.
 *
 * @param mem_base Destination buffer for received data.
 * @param length Number of bytes to receive.
 * @return Zero on success, or a negative error code on failure.
 */
int sunxi_usb_start_recv_by_dma(void *mem_base, uint32_t length);

/**
 * @brief Return the status of the most recent DMA receive transfer.
 *
 * @return Zero when the transfer completed without a tail error, otherwise a
 * negative error code.
 */
int sunxi_usb_get_dma_rx_status(void);

/**
 * @brief Send a control transfer response through endpoint zero.
 *
 * @param length Number of response bytes.
 * @param buffer Response data, or NULL for a zero-length response.
 * @return Zero on success, or a negative error code on failure.
 */
int sunxi_usb_send_setup(uint32_t length, const void *buffer);

/**
 * @brief Complete a USB SET_ADDRESS request.
 *
 * @param address The USB device address in the range 0 through 127.
 * @return A USB request status code.
 */
int sunxi_usb_set_address(uint32_t address);

/**
 * @brief Send data through the configured bulk IN endpoint.
 *
 * @param buffer Source data, or NULL for a zero-length transfer.
 * @param buffer_size Number of bytes to send.
 * @return Zero on success, or a negative error code on failure.
 */
int sunxi_usb_send_data(void *buffer, uint32_t buffer_size);

/** @brief Return the negotiated bulk endpoint maximum packet size. */
int sunxi_usb_get_ep_max(void);

/** @brief Return the address of the configured bulk IN endpoint. */
int sunxi_usb_get_ep_in_type(void);

/** @brief Return the address of the configured bulk OUT endpoint. */
int sunxi_usb_get_ep_out_type(void);

#endif /* __DRIVERS_USB_MANAGER_H__ */
