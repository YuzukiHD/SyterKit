/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_MANAGER_H__
#define __DRIVERS_USB_MANAGER_H__

#include <drivers/usb/usb_types.h>

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

void sunxi_usb_attach_module(uint32_t device_type);
int sunxi_usb_init(void);
void sunxi_usb_dump(uint32_t usbc_base, uint32_t ep_index);
void sunxi_usb_ep_reset(void);
void sunxi_usb_irq(void *data);
void sunxi_usb_attach(void);
int sunxi_usb_extern_loop(void);
void sunxi_usb_bulk_ep_reset(void);
int sunxi_usb_start_recv_by_dma(void *mem_base, uint32_t length);
int sunxi_usb_get_dma_rx_status(void);
int sunxi_usb_send_setup(uint32_t length, const void *buffer);
int sunxi_usb_set_address(uint32_t address);
int sunxi_usb_send_data(void *buffer, uint32_t buffer_size);
int sunxi_usb_get_ep_max(void);
int sunxi_usb_get_ep_in_type(void);
int sunxi_usb_get_ep_out_type(void);

#endif /* __DRIVERS_USB_MANAGER_H__ */
