/* SPDX-License-Identifier:	GPL-2.0+ */
/* Based on https://github.com/allwinner-zh/bootloader */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <common.h>
#include <log.h>
#include <mmu.h>

#include <drivers/clk/clk.h>
#include <drivers/intc/intc.h>
#include <dt-compatible/usb-dt.h>
#include <interrupt.h>
#include <timer.h>

#include <drivers/usb/function/usb_function.h>
#include <drivers/usb/platform/usb_platform.h>
#include <drivers/usb/usb_controller.h>
#include <drivers/usb/usb_device.h>
#include <drivers/usb/usb_lowlevel.h>
#include <drivers/usb/usb_manager.h>

#define SUNXI_USB_EP0_BUFFER_SIZE     (512)
#define HIGH_SPEED_EP_MAX_PACKET_SIZE (512)
#define FULL_SPEED_EP_MAX_PACKET_SIZE (64)
#define BULK_FIFOSIZE		      (512)
#define RX_BUFF_SIZE		      (512)
#define SUNXI_USB_CTRL_EP_INDEX	      0
#define SUNXI_USB_BULK_IN_EP_INDEX    1 /* tx */
#define SUNXI_USB_BULK_OUT_EP_INDEX   2 /* rx */
#define SUNXI_USB_WRITE_TIMEOUT_MS    100U

static uint8_t sunxi_usb_ep0_buffer[SUNXI_USB_EP0_BUFFER_SIZE];

static sunxi_ubuf_t sunxi_ubuf;
static sunxi_udc_t sunxi_udc_source;
static const sunxi_usb_function_ops_t *sunxi_udev_active;

static uint8_t usb_dma_trans_unaliged_bytes;
static uint8_t *usb_dma_trans_unaligned_buf;

static volatile uint8_t usb_rx_deferred;
static volatile uint8_t usb_dma_rx_active;
static volatile uint8_t usb_dma_rx_error;

static sunxi_usb_t sunxi_usb_config;

static void sunxi_usb_clock_deinit(void)
{
	clrbits_le32(sunxi_usb_config.clock_gate_reg_base, BIT(sunxi_usb_config.reset_offset));
	clrbits_le32(sunxi_usb_config.clock_gate_reg_base, BIT(sunxi_usb_config.clock_gate_offset));
	clrbits_le32(sunxi_usb_config.phy_clock_reg_base, BIT(sunxi_usb_config.phy_reset_offset));
	clrbits_le32(sunxi_usb_config.phy_clock_reg_base, BIT(sunxi_usb_config.phy_clock_gate_offset));
}

static void sunxi_usb_clock_init(void)
{
	setbits_le32(sunxi_usb_config.phy_clock_reg_base, BIT(sunxi_usb_config.phy_clock_gate_offset));
	setbits_le32(sunxi_usb_config.phy_clock_reg_base, BIT(sunxi_usb_config.phy_reset_offset));
	setbits_le32(sunxi_usb_config.clock_gate_reg_base, BIT(sunxi_usb_config.reset_offset));
	setbits_le32(sunxi_usb_config.clock_gate_reg_base, BIT(sunxi_usb_config.clock_gate_offset));
}

/**
 * @brief Clear all USB interrupts.
 *
 * This function clears all pending USB interrupts, including endpoint transmit (TX) and receive (RX) interrupts, as well as miscellaneous interrupts.
 */
static void sunxi_usb_clear_all_irq(void)
{
	usb_controller_int_clear_ep_pending_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);
	usb_controller_int_clear_ep_pending_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
	usb_controller_int_clear_misc_pending_all(sunxi_udc_source.usbc_hd);
}

/* Stop in-flight transfers before resetting software and endpoint state. */
static void sunxi_usb_reset_transfer_state(bool notify_function)
{
	uint32_t old_ep_idx;

	usb_dma_rx_active = 0;
	usb_dma_rx_error = 0;
	usb_rx_deferred = 0;
	usb_dma_trans_unaliged_bytes = 0;
	usb_dma_trans_unaligned_buf = NULL;
	sunxi_ubuf.rx_req_length = 0;
	sunxi_ubuf.rx_ready_for_data = 0;
	sunxi_ubuf.request_size = 0;
	sunxi_udc_source.address = 0;
	sunxi_udc_source.speed = USB_SPEED_HIGH;
	sunxi_udc_source.bulk_ep_max = HIGH_SPEED_EP_MAX_PACKET_SIZE;
	sunxi_udc_source.fifo_size = BULK_FIFOSIZE;
	sunxi_udc_source.ep0_stage = 0;
	memset(&sunxi_udc_source.standard_reg, 0, sizeof(sunxi_udc_source.standard_reg));

	if (sunxi_udc_source.usbc_hd == 0)
		goto notify;

	old_ep_idx = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
	if (sunxi_udc_source.dma_recv_channal)
		usb_dma_stop(sunxi_udc_source.dma_recv_channal);
	if (sunxi_udc_source.dma_send_channal)
		usb_dma_stop(sunxi_udc_source.dma_send_channal);
	if (sunxi_udc_source.dma_send_channal)
		usb_dma_set_pktlen(sunxi_udc_source.dma_send_channal, sunxi_udc_source.bulk_ep_max);
	if (sunxi_udc_source.dma_recv_channal)
		usb_dma_set_pktlen(sunxi_udc_source.dma_recv_channal, sunxi_udc_source.bulk_ep_max);

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_IN_EP_INDEX);
	usb_device_clear_ep_dma(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);
	usb_device_flush_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);
	usb_device_clear_ep_dma(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
	usb_device_flush_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_CTRL_EP_INDEX);
	usb_device_flush_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
	usb_device_set_address_default(sunxi_udc_source.usbc_hd);
	if (sunxi_ubuf.rx_req_buffer != NULL && sunxi_ubuf.rx_buffer_size != 0U)
		memset(sunxi_ubuf.rx_req_buffer, 0, sunxi_ubuf.rx_buffer_size);
	sunxi_usb_clear_all_irq();
	usb_dma_int_clear();
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);

notify:
	if (notify_function && sunxi_udev_active != NULL && sunxi_udev_active->state_reset != NULL)
		sunxi_udev_active->state_reset();
}

/**
 * @brief Handle the completion of a USB data read operation.
 *
 * This function handles the completion of a USB data read operation, updating the status and clearing any relevant interrupts.
 *
 * @param husb The USB controller handle.
 * @param ep_type The type of endpoint where the read operation was performed.
 * @param complete A flag indicating whether the read operation completed successfully or not.
 */
static void sunxi_usb_read_complete(uintptr_t husb, uint32_t ep_type, uint32_t complete)
{
	usb_device_read_data_status(husb, ep_type, complete);
	if (ep_type == USBC_EP_TYPE_EP0) {
		/* clear data end */
		if (complete) {
			usb_device_ep0_clear_setup_end(husb);
		}
		/* clear irq */
		usb_controller_int_clear_ep_pending(husb, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);
	}
	return;
}

/**
 * @brief Handle USB write complete event
 *
 * This function is called when a USB write operation is completed. It updates
 * the write data status using usb_device_write_data_status(). EP0 completion
 * is handled asynchronously by the next TX interrupt, while bulk transfers
 * wait for the FIFO to become available before sending the next packet.
 *
 * @param husb     The USB handle
 * @param ep_type  The type of the endpoint
 * @param complete The completion status of the write operation
 * @return 0 on success, or -1 if the status update or bulk TX wait fails
 */
static int sunxi_usb_write_complete(uintptr_t husb, uint32_t ep_type, uint32_t complete)
{
	uint32_t start_time;
	uint32_t write_ready;
	int ret;

	ret = usb_device_write_data_status(husb, ep_type, complete);
	if (ret != 0)
		return ret;

	if (ep_type == USBC_EP_TYPE_EP0) {
		/* Clear data end */
		if (complete) {
			usb_device_ctrl_clear_setup_end(husb);
		}

		/* Clear IRQ */
		usb_controller_int_clear_ep_pending(husb, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);
		return 0;
	}

	/* EP0 is completed from the next TX interrupt. Bulk TX must wait here
	 * because the caller may immediately fill the same FIFO again. */
	start_time = time_ms();
	while ((write_ready = usb_device_get_write_data_ready(husb, ep_type)) != 0) {
		if (time_ms() - start_time >= SUNXI_USB_WRITE_TIMEOUT_MS) {
			pr_warn("usb: ep%u write complete timed out waiting for TX ready\n",
				usb_controller_get_active_ep(husb));
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Write data to the USB FIFO
 *
 * This function writes data from the buffer to the USB FIFO. It selects the
 * appropriate endpoint, calculates the transfer size, and repeatedly calls
 * usb_controller_write_packet() to write data to the FIFO until all data is
 * transferred. After each write, it calls sunxi_usb_write_complete() to handle
 * the completion of the write operation.
 *
 * @param buffer       The buffer containing the data to be written
 * @param buffer_size  The size of the buffer
 * @return 0 on success, -1 on failure
 */
static int sunxi_usb_write_fifo(uint8_t *buffer, uint32_t buffer_size)
{
	uint32_t old_ep_idx = 0;
	uint32_t fifo = 0;
	uint32_t transfered = 0;
	uint32_t left = 0;
	uint32_t this_len;
	int ret = 0;

	/* Save index */
	old_ep_idx = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_IN_EP_INDEX);

	left = buffer_size;
	fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_IN_EP_INDEX);

	while (left) {
		this_len = min(sunxi_udc_source.fifo_size, left);
		printk_trace("USB: tx packet ep=%u len=0x%x buf=0x%x\n", SUNXI_USB_BULK_IN_EP_INDEX, this_len,
			(uint32_t)(uintptr_t)(buffer + transfered));
		this_len = usb_controller_write_packet(sunxi_udc_source.usbc_hd, fifo, this_len, buffer + transfered);
		if (!this_len) {
			ret = -1;
			break;
		}
		transfered += this_len;
		left -= this_len;
		if (sunxi_usb_write_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, 1) != 0) {
			ret = -1;
			break;
		}
	}

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
	return ret;
}

/**
 * @brief Read data from endpoint 0 of the USB device.
 *
 * This function reads data from endpoint 0 of the USB device and stores it in the provided buffer.
 *
 * @param buffer Pointer to the buffer where the read data will be stored.
 * @param buffer_size Capacity of the destination buffer.
 * @param setup_packet Whether the packet must be an 8-byte USB setup packet.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int sunxi_usb_read_ep0_data(void *buffer, uint32_t buffer_size, bool setup_packet)
{
	uint32_t fifo_count = 0;
	uint32_t fifo = 0;
	int ret = 0;
	uint32_t old_ep_index = 0;

	if (buffer == NULL || buffer_size == 0U)
		return -1;

	old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
	fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_CTRL_EP_INDEX);
	fifo_count = usb_controller_read_len_from_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
	if (setup_packet && fifo_count != sizeof(struct usb_device_request)) {
		printk_error("USB: ep0 setup length %u is not %u\n", fifo_count,
			(uint32_t)sizeof(struct usb_device_request));
		ret = -1;
		goto out;
	}
	if (fifo_count > buffer_size) {
		printk_error("USB: ep0 packet length %u exceeds buffer size %u\n", fifo_count, buffer_size);
		ret = -1;
		goto out;
	}
	if (usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, fifo_count, buffer) != fifo_count) {
		ret = -1;
		goto out;
	}
	sunxi_usb_read_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0, 1);

out:
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
	return ret;
}

/**
 * @brief Set the USB device address.
 *
 * This function sets the USB device address and updates the speed and FIFO size
 * based on the transfer mode of the USB controller.
 *
 * @param address The USB device address to be set.
 *
 * @return Returns SUNXI_USB_REQ_SUCCESSED on success, or a negative error code on failure.
 */
static int sunxi_usb_perform_set_address(uint8_t address)
{
	uint32_t packet_size;

	usb_device_set_address(sunxi_udc_source.usbc_hd, address);
	if (usb_device_query_transfer_mode(sunxi_udc_source.usbc_hd) == USBC_TS_MODE_HS) {
		sunxi_udc_source.speed = USB_SPEED_HIGH;
		packet_size = HIGH_SPEED_EP_MAX_PACKET_SIZE;
		printk_trace("USB: usb speed: HIGH\n");
	} else {
		sunxi_udc_source.speed = USB_SPEED_FULL;
		packet_size = FULL_SPEED_EP_MAX_PACKET_SIZE;
		printk_trace("USB: usb speed: FULL\n");
	}
	sunxi_udc_source.fifo_size = packet_size;
	sunxi_udc_source.bulk_ep_max = packet_size;
	if (usb_dma_set_pktlen(sunxi_udc_source.dma_send_channal, packet_size) != 0 ||
		usb_dma_set_pktlen(sunxi_udc_source.dma_recv_channal, packet_size) != 0)
		return SUNXI_USB_REQ_OP_ERR;
	return SUNXI_USB_REQ_SUCCESSED;
}

/**
 * @brief Receive and process data on EP0.
 *
 * This function receives and processes data on Endpoint 0 (EP0).
 * It clears stall status, checks if setup end is cleared,
 * and checks if the read data on EP0 is ready.
 * Depending on the type of USB request, it performs different operations.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int ep0_recv_op(void)
{
	uint32_t old_ep_index = 0;
	int ret = 0;

	if (!sunxi_udc_source.ep0_stage) {
		memset(&sunxi_udc_source.standard_reg, 0, sizeof(struct usb_device_request));
	}

	old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_CTRL_EP_INDEX);

	/* clear stall status */
	if (usb_device_get_ep_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0)) {
		printk_error("USB: handle_ep0: ep0 stall\n");
		usb_device_ep_clear_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
		ret = -1;
		goto ep0_recv_op_err;
	}

	/* clear setup end */
	if (usb_device_ctrl_get_setup_end(sunxi_udc_source.usbc_hd)) {
		usb_device_ctrl_clear_setup_end(sunxi_udc_source.usbc_hd);
		sunxi_udc_source.ep0_stage = 0;
	}

	/* Check if read data on EP0 is ready */
	if (usb_device_get_read_data_ready(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0)) {
		uint32_t status;

		if (!sunxi_udc_source.ep0_stage) {
			status = sunxi_usb_read_ep0_data(
				&sunxi_udc_source.standard_reg, sizeof(sunxi_udc_source.standard_reg), true);
		} else {
			status = sunxi_usb_read_ep0_data(sunxi_usb_ep0_buffer, sizeof(sunxi_usb_ep0_buffer), false);
		}
		if (status != 0) {
			printk_error("USB: read_request failed\n");
			sunxi_udc_source.ep0_stage = 0;
			usb_device_ep_send_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
			ret = -1;
			goto ep0_recv_op_err;
		}
	} else {
		/* This case is usually caused by sending an empty packet on EP0, can be ignored */
		printk_trace("USB: ep0 rx non, set addr\n");
		if (sunxi_udc_source.address) {
			sunxi_usb_perform_set_address(sunxi_udc_source.address & 0xff);
			printk_debug("USB: set address 0x%x ok\n", sunxi_udc_source.address);
			sunxi_udc_source.address = 0;
		}
		sunxi_udc_source.ep0_stage = 0;
		goto ep0_recv_op_err;
	}
	/* Check data */
	if (USB_REQ_TYPE_STANDARD == (sunxi_udc_source.standard_reg.request_type & USB_REQ_TYPE_MASK)) {
		ret = SUNXI_USB_REQ_UNMATCHED_COMMAND;
		/* standard */
		switch (sunxi_udc_source.standard_reg.request) {
		case USB_REQ_GET_STATUS: /*   0x00*/
		{
			/* device-to-host */
			if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				ret = sunxi_udev_active->standard_req_op(
					USB_REQ_GET_STATUS, &sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
			}
			break;
		}
		case USB_REQ_CLEAR_FEATURE: /*   0x01*/
		{
			/* host-to-device */
			if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				ret = sunxi_udev_active->standard_req_op(
					USB_REQ_CLEAR_FEATURE, &sunxi_udc_source.standard_reg, NULL);
			}
			break;
		}
		case USB_REQ_SET_FEATURE: /*   0x03*/
		{
			/* host-to-device */
			if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				ret = sunxi_udev_active->standard_req_op(
					USB_REQ_SET_FEATURE, &sunxi_udc_source.standard_reg, NULL);
			}

			break;
		}
		case USB_REQ_SET_ADDRESS: /*   0x05*/
		{
			/* host-to-device */
			if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_DEVICE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					/* receiver is device */
					ret = sunxi_udev_active->standard_req_op(
						USB_REQ_SET_ADDRESS, &sunxi_udc_source.standard_reg, NULL);
				}
			}

			break;
		}
		case USB_REQ_GET_DESCRIPTOR: /*   0x06*/
		{
			/* device-to-host */
			if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_DEVICE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_DESCRIPTOR,
						&sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
				}
			}

			break;
		}
		case USB_REQ_SET_DESCRIPTOR: /*   0x07*/
		{
			/* host-to-device */
			if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_DEVICE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					/*there is some problem*/
					ret = sunxi_udev_active->standard_req_op(USB_REQ_SET_DESCRIPTOR,
						&sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
				}
			}

			break;
		}
		case USB_REQ_GET_CONFIGURATION: /*   0x08*/
		{
			/* device-to-host */
			if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_DEVICE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_CONFIGURATION,
						&sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
				}
			}

			break;
		}
		case USB_REQ_SET_CONFIGURATION: /*   0x09*/
		{
			/* host-to-device */
			if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_DEVICE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					ret = sunxi_udev_active->standard_req_op(
						USB_REQ_SET_CONFIGURATION, &sunxi_udc_source.standard_reg, NULL);
				}
			}

			break;
		}
		case USB_REQ_GET_INTERFACE: /*   0x0a*/
		{
			/* device-to-host */
			if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_INTERFACE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_INTERFACE,
						&sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
				}
			}

			break;
		}
		case USB_REQ_SET_INTERFACE: /*   0x0b*/
		{
			/* host-to-device */
			if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_INTERFACE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					ret = sunxi_udev_active->standard_req_op(
						USB_REQ_SET_INTERFACE, &sunxi_udc_source.standard_reg, NULL);
				}
			}

			break;
		}
		case USB_REQ_SYNCH_FRAME: /*   0x0c*/
		{
			/* device-to-host */
			if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
				if (USB_RECIP_INTERFACE ==
					(sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
					ret = sunxi_udev_active->standard_req_op(
						USB_REQ_SYNCH_FRAME, &sunxi_udc_source.standard_reg, NULL);
				}
			}

			break;
		}
		default: {
			printk_error("USB: sunxi usb err: unknown usb out request to device\n");
			ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
			break;
		}
		}
	} else {
		/* Non-Standard Req */
		printk_trace("USB: non standard req\n");
		ret = sunxi_udev_active->nonstandard_req_op(USB_REQ_GET_STATUS, &sunxi_udc_source.standard_reg,
			sunxi_usb_ep0_buffer, sunxi_udc_source.ep0_stage);
		if (ret == SUNXI_USB_REQ_DATA_HUNGRY) {
			sunxi_udc_source.ep0_stage = 1;
		} else if (ret == SUNXI_USB_REQ_SUCCESSED) {
			sunxi_udc_source.ep0_stage = 0;
		} else if (ret < 0) {
			sunxi_udc_source.ep0_stage = 0;
			printk_error("USB: unkown request_type(%d)\n", sunxi_udc_source.standard_reg.request_type);
		}
	}
	if (ret < 0 && ret != SUNXI_USB_REQ_DATA_HUNGRY) {
		sunxi_udc_source.ep0_stage = 0;
		usb_device_ep_send_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
	}

ep0_recv_op_err:
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
	return ret;
}

/**
 * @brief Send data on EP0.
 *
 * This function send data on Endpoint 0 (EP0).
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int eptx_send_op()
{
	// TODO: ep send operating
	return 0;
}

/*
 * Function name: sunxi_usb_recv_by_dma_isr
 *
 * @param p_arg: Pointer to the argument passed to the ISR. (Input)
 *
 * Description:
 * This function is the interrupt service routine for USB receive by DMA.
 * It takes a pointer to the argument passed to the ISR as an input parameter.
 * The function selects the RX endpoint, clears the DMA flag, and transfers the data using IO mode.
 * If there are unaligned bytes in the DMA transfer, it reads the packet manually and returns the status.
 * If the current DMA transfer is not a complete packet, it manually clears the interrupt.
 * Lastly, it selects the previous active endpoint and calls the RX DMA ISR of the active USB device.
 */
static void sunxi_usb_recv_by_dma_isr(void *p_arg)
{
	uint32_t old_ep_idx = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
	uint32_t tail_length = usb_dma_trans_unaliged_bytes;
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX); /* Select RXEP */
	usb_dma_rx_active = 0;

	/* Select IO mode for the data transfer */
	printk_trace("USB: select io mode to transfer data\n");
	if (usb_device_clear_ep_dma(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX) != 0)
		printk_error("USB: failed to clear RX DMA mode\n");

	if (tail_length) {
		uint32_t fifo;
		uint32_t fifo_count;
		uint32_t this_len;

		fifo_count = usb_controller_read_len_from_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
		if (fifo_count != tail_length) {
			printk_error("USB: DMA tail length %u, expected %u\n", fifo_count, tail_length);
			usb_dma_rx_error = 1;
		}
		this_len = min(fifo_count, tail_length);
		fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);
		if (this_len) {
			if (usb_dma_trans_unaligned_buf == NULL ||
				usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, this_len,
					usb_dma_trans_unaligned_buf) != this_len) {
				printk_error("USB: failed to read DMA tail\n");
				usb_dma_rx_error = 1;
			}
		}
		sunxi_usb_read_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1); /* Return status */
	}
	usb_dma_trans_unaliged_bytes = 0;
	usb_dma_trans_unaligned_buf = NULL;

	/* If the current DMA transfer is not a complete packet, manually clear the interrupt */
	if (sunxi_udc_source.bulk_ep_max != 0U && sunxi_ubuf.request_size % sunxi_udc_source.bulk_ep_max) {
		usb_device_read_data_status(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1);
	}
	sunxi_ubuf.request_size = 0;

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
	if (sunxi_udev_active != NULL && sunxi_udev_active->dma_rx_isr != NULL)
		sunxi_udev_active->dma_rx_isr(p_arg);
}

/*
 * Function name: sunxi_usb_send_by_dma_isr
 *
 * Description:
 * This function is the interrupt service routine for sending data using DMA.
 * It calls the DMA transmit interrupt service routine specified by 'dma_tx_isr' of the active USB device.
 *
 * Parameters:
 * - p_arg: A void pointer that can be used to pass arguments to the DMA transmit interrupt service routine.
 */
static void sunxi_usb_send_by_dma_isr(void *p_arg)
{
	if (sunxi_udev_active != NULL && sunxi_udev_active->dma_tx_isr != NULL)
		sunxi_udev_active->dma_tx_isr(p_arg);
}

/*
 * Function name: eprx_recv_op
 *
 * Description:
 * This function performs the USB receive operation on the RX endpoint. It selects the RX endpoint, checks if it is stalled,
 * and clears the stall if necessary. If the data is ready to be read, it reads the length from the FIFO and checks if DMA is in progress.
 * If DMA is in progress, it reads the packet manually and returns the status.
 * Otherwise, if the buffer is not ready to receive data, it reads the data into the buffer and sets the buffer flag.
 * If the buffer is already set, it does nothing and leaves it to DMA.
 * Lastly, it selects the previous active endpoint and returns 0.
 *
 * Return:
 * 0 - Success
 */
static int eprx_recv_op(void)
{
	uint32_t old_ep_index;
	uint32_t this_len;
	uint32_t fifo;
	int ret = 0;

	old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);

	if (usb_device_get_ep_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX)) {
		usb_device_ep_clear_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
		printk_error("sunxi ubs read error: usb rx ep is busy already\n");
	} else {
		if (usb_device_get_read_data_ready(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX)) {
			this_len = usb_controller_read_len_from_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
			if (this_len > sunxi_udc_source.bulk_ep_max || this_len > sunxi_ubuf.rx_buffer_size ||
				sunxi_ubuf.rx_req_buffer == NULL) {
				printk_error("USB: RX packet length %u exceeds buffer or endpoint limit\n", this_len);
				usb_device_flush_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
				usb_device_ep_send_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
				ret = -1;
				goto out;
			} else if (!sunxi_ubuf.rx_ready_for_data) {
				fifo = usb_controller_select_fifo(
					sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);

				memset(sunxi_ubuf.rx_req_buffer, 0, sunxi_ubuf.rx_buffer_size);
				sunxi_ubuf.rx_req_length = usb_controller_read_packet(
					sunxi_udc_source.usbc_hd, fifo, this_len, sunxi_ubuf.rx_req_buffer);
				if (sunxi_ubuf.rx_req_length != this_len) {
					usb_device_flush_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
					ret = -1;
					goto out;
				}
				sunxi_ubuf.rx_ready_for_data = 1;

				printk_trace("USB: read ep bytes 0x%x\n", sunxi_ubuf.rx_req_length);
				sunxi_usb_read_complete(
					sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1); /* Return status */
			} else {
				printk_trace("USB: eprx do nothing and left it to dma\n");
			}
		} else {
			printk_trace("USB: sunxi usb rxdata not ready\n");
		}
	}
out:
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
	return ret;
}

/* Revisit a packet whose endpoint IRQ arrived while rx_ready_for_data was set. */
static void sunxi_usb_process_deferred_rx(void)
{
	if (!usb_rx_deferred || usb_dma_rx_active || sunxi_ubuf.rx_ready_for_data)
		return;

	usb_rx_deferred = 0;
	eprx_recv_op();
}

void sunxi_usb_attach_module(uint32_t device_type)
{
	const sunxi_usb_function_t *function = sunxi_usb_function_lookup(device_type);

	if (function == NULL || function->ops == NULL) {
		printk_error("USB: device function %u is not registered\n", device_type);
		return;
	}

	sunxi_udev_active = function->ops;
	printk_info("USB: selected device function %s\n", function->name);
}

int sunxi_usb_init()
{
	uint32_t reg_val = 0;
	static uint8_t rx_base_buffer[RX_BUFF_SIZE];

	if (sunxi_usb_dt_read_alias(&sunxi_usb_config, "usb0") != DRIVER_OK || sunxi_usb_config.irq == 0U) {
		printk_error("USB: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_usb_platform_init(&sunxi_usb_config) != DRIVER_OK) {
		printk_error("USB: platform initialization failed\n");
		return -1;
	}

	if (usb_controller_set_base(sunxi_usb_config.id, (uint32_t)sunxi_usb_config.base) != 0) {
		printk_error("USB: invalid controller configuration\n");
		goto sunxi_usb_init_platform_fail;
	}

	if (sunxi_udev_active == NULL) {
		printk_error("USB: no device module attached\n");
		goto sunxi_usb_init_platform_fail;
	}

	if (sunxi_udev_active->state_init()) {
		printk_error("USB: fail to init usb device\n");
		goto sunxi_usb_init_platform_fail;
	}

	if (irq_disable((int)sunxi_usb_config.irq) != DRIVER_OK) {
		printk_error("USB: failed to disable IRQ %u\n", sunxi_usb_config.irq);
		goto sunxi_usb_init_platform_fail;
	}

	printk_trace("Init udc controller source\n");
	/* Init udc controller source */
	memset(&sunxi_udc_source, 0, sizeof(sunxi_udc_t));

	sunxi_udc_source.usbc_hd = usb_controller_open_otg(sunxi_usb_config.id);

	if (sunxi_udc_source.usbc_hd == 0) {
		printk_error("USB: usb_controller_open_otg failed\n");
		goto sunxi_usb_init_fail;
	}

	if (usb_dma_init(sunxi_udc_source.usbc_hd) != 0) {
		printk_error("USB: failed to initialize DMA\n");
		goto sunxi_usb_init_fail;
	}

	usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_OFF);

	/* Close clock */
	sunxi_usb_clock_deinit();

	/* request dma channal for usb send and recv */
	sunxi_udc_source.dma_send_channal = usb_dma_request();
	if (!sunxi_udc_source.dma_send_channal) {
		printk_error("USB: unable to request dma for usb send data\n");
		goto sunxi_usb_init_fail;
	}
	printk_trace("USB: dma send ch %d\n", sunxi_udc_source.dma_send_channal);
	sunxi_udc_source.dma_recv_channal = usb_dma_request();
	if (!sunxi_udc_source.dma_recv_channal) {
		printk_error("USB: unable to request dma for usb receive data\n");
		goto sunxi_usb_init_fail;
	}
	printk_trace("USB: dma recv ch %d\n", sunxi_udc_source.dma_recv_channal);

	/* init usb info */
	sunxi_udc_source.address = 0;
	sunxi_udc_source.speed = USB_SPEED_HIGH;
	sunxi_udc_source.bulk_ep_max = HIGH_SPEED_EP_MAX_PACKET_SIZE;
	sunxi_udc_source.fifo_size = BULK_FIFOSIZE;
	sunxi_udc_source.bulk_in_addr = 100;
	sunxi_udc_source.bulk_out_addr = sunxi_udc_source.bulk_in_addr + sunxi_udc_source.fifo_size * 2;

	/* init usb buffer */
	memset(&sunxi_ubuf, 0, sizeof(sunxi_ubuf_t));
	usb_dma_rx_active = 0;
	usb_dma_rx_error = 0;
	usb_rx_deferred = 0;
	usb_dma_trans_unaliged_bytes = 0;
	usb_dma_trans_unaligned_buf = NULL;

	/* We use static memory for this moment */
	sunxi_ubuf.rx_base_buffer = rx_base_buffer;
	if (!sunxi_ubuf.rx_base_buffer) {
		printk_error("USB: RX buffer allocation failed\n");
		goto sunxi_usb_init_fail;
	}
	sunxi_ubuf.rx_req_buffer = sunxi_ubuf.rx_base_buffer;
	sunxi_ubuf.rx_buffer_size = sizeof(rx_base_buffer);

	/* open usb clock */
	sunxi_usb_clock_init();

	/* disable OTG ID detect and set to device */
	usb_controller_force_id_status(sunxi_udc_source.usbc_hd, USBC_ID_TYPE_DEVICE);

	/* Force VBUS to HIGH */
	usb_controller_force_vbus_valid(sunxi_udc_source.usbc_hd, USBC_VBUS_TYPE_HIGH);

	/* disconnect usb */
	usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_OFF);

	/* set pull up for dp dm and id */
	usb_controller_dpdm_pull_enable(sunxi_udc_source.usbc_hd);
	usb_controller_id_pull_enable(sunxi_udc_source.usbc_hd);

	/* Set to use PIO mode */
	usb_controller_select_bus(sunxi_udc_source.usbc_hd, USBC_IO_TYPE_PIO, 0, 0);

	/* mapping SRAM buffer */
	usb_controller_config_fifo_base(sunxi_udc_source.usbc_hd, 0);

	/* set usb to HS mode and Bulk */
	usb_device_config_transfer_mode(sunxi_udc_source.usbc_hd, USBC_TS_TYPE_BULK, USBC_TS_MODE_HS);

	/* config dma for send */
	if (usb_dma_setting(sunxi_udc_source.dma_send_channal, USB_DMA_FROM_DRAM_TO_HOST, SUNXI_USB_BULK_IN_EP_INDEX) !=
			0 ||
		usb_dma_set_pktlen(sunxi_udc_source.dma_send_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE) != 0) {
		printk_error("USB: failed to configure TX DMA\n");
		goto sunxi_usb_init_fail;
	}

	/* config dma for recv */
	if (usb_dma_setting(
		    sunxi_udc_source.dma_recv_channal, USB_DMA_FROM_HOST_TO_DRAM, SUNXI_USB_BULK_OUT_EP_INDEX) != 0 ||
		usb_dma_set_pktlen(sunxi_udc_source.dma_recv_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE) != 0) {
		printk_error("USB: failed to configure RX DMA\n");
		goto sunxi_usb_init_fail;
	}

	/* disable all interrupt */
	usb_controller_int_disable_usb_misc_all(sunxi_udc_source.usbc_hd);
	usb_controller_int_disable_ep_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
	usb_controller_int_disable_ep_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);

	/* enable reset, disconnect, resume, suspend interrupt */
	usb_controller_int_enable_usb_misc_uint(sunxi_udc_source.usbc_hd, USBC_INTUSB_DISCONNECT | USBC_INTUSB_SUSPEND |
										  USBC_INTUSB_RESUME |
										  USBC_INTUSB_RESET | USBC_INTUSB_SOF);

	/* enable ep interrupt */
	usb_controller_int_enable_ep(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);

	/* reset all ep */
	sunxi_usb_bulk_ep_reset();

	/* open usb device */
	usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_ON);

	irq_install_handler((int)sunxi_usb_config.irq, sunxi_usb_irq, NULL);
	if (irq_enable((int)sunxi_usb_config.irq) != DRIVER_OK) {
		printk_error("USB: failed to enable IRQ %u\n", sunxi_usb_config.irq);
		goto sunxi_usb_init_fail;
	}

	/* set bit 1  ->  0 */
	reg_val = readl(sunxi_usb_config.base + USBC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << 1);
	writel(reg_val, sunxi_usb_config.base + USBC_REG_o_PHYCTL);

	reg_val = readl(sunxi_usb_config.base + USBC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, sunxi_usb_config.base + USBC_REG_o_PHYCTL);

	/* sunxi_usb_dump(sunxi_usb_config.base, 0); // for debug */

	return 0;

sunxi_usb_init_fail:
	irq_disable((int)sunxi_usb_config.irq);
	if (sunxi_udc_source.dma_send_channal) {
		usb_dma_release(sunxi_udc_source.dma_send_channal);
	}
	if (sunxi_udc_source.dma_recv_channal) {
		usb_dma_release(sunxi_udc_source.dma_recv_channal);
	}
	if (sunxi_udc_source.usbc_hd) {
		usb_controller_close_otg(sunxi_udc_source.usbc_hd);
	}
	sunxi_usb_clock_deinit();
	sunxi_usb_platform_deinit(&sunxi_usb_config);
	return -1;

sunxi_usb_init_platform_fail:
	sunxi_usb_platform_deinit(&sunxi_usb_config);
	return -1;
}

void sunxi_usb_ep_reset(void)
{
	sunxi_usb_bulk_ep_reset();
}

void sunxi_usb_irq(void *data)
{
	uint8_t misc_irq = 0;
	uint16_t tx_irq = 0;
	uint16_t rx_irq = 0;
	uint32_t dma_irq = 0;
	int dma_status;
	uint32_t old_ep_idx;

	(void)data;

	/* Save index */
	old_ep_idx = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);

	/* Read status registers */
	misc_irq = usb_controller_int_misc_pending(sunxi_udc_source.usbc_hd);
	tx_irq = usb_controller_int_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);
	rx_irq = usb_controller_int_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
	dma_status = usb_dma_int_query();
	if (dma_status >= 0)
		dma_irq = (uint32_t)dma_status;

	/* RESET */
	if (misc_irq & USBC_INTUSB_RESET) {
		usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_RESET);

		usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, 0);
		usb_device_set_address_default(sunxi_udc_source.usbc_hd);
		sunxi_usb_reset_transfer_state(true);
		sunxi_usb_bulk_ep_reset();

		return;
	}

	/* RESUME ont handle, just clear interrupt */
	if (misc_irq & USBC_INTUSB_RESUME) {
		/* clear interrupt */
		usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_RESUME);
	}

	/* SUSPEND */
	if (misc_irq & USBC_INTUSB_SUSPEND) {
		/* clear interrupt */
		usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_SUSPEND);
	}

	/* DISCONNECT */
	if (misc_irq & USBC_INTUSB_DISCONNECT) {
		usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_DISCONNECT);
		sunxi_usb_reset_transfer_state(true);
		return;
	}

	/* SOF */
	if (misc_irq & USBC_INTUSB_SOF) {
		usb_controller_int_disable_usb_misc_uint(sunxi_udc_source.usbc_hd, USBC_INTUSB_SOF);
		usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_SOF);
	}

	/* ep0 */
	if (tx_irq & (1 << SUNXI_USB_CTRL_EP_INDEX)) {
		usb_controller_int_clear_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);
		/* handle ep0 ops */
		ep0_recv_op();
	}

	/* tx endpoint data transfers */
	if (tx_irq & (1 << SUNXI_USB_BULK_IN_EP_INDEX)) {
		/* Clear the interrupt bit by setting it to 1 */
		usb_controller_int_clear_ep_pending(
			sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_BULK_IN_EP_INDEX);
		eptx_send_op();
	}

	/* rx endpoint data transfers */
	if (rx_irq & (1 << SUNXI_USB_BULK_OUT_EP_INDEX)) {
		/* Clear the interrupt bit by setting it to 1 */
		usb_controller_int_clear_ep_pending(
			sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, SUNXI_USB_BULK_OUT_EP_INDEX);
		if (sunxi_ubuf.rx_ready_for_data) {
			/* A second packet arrived before the main loop consumed the first one. */
			usb_rx_deferred = 1;
		} else if (!usb_dma_rx_active) {
			eprx_recv_op();
		}
	}

	if (sunxi_udc_source.dma_send_channal < 32U && dma_irq & (1U << sunxi_udc_source.dma_send_channal)) {
		sunxi_usb_send_by_dma_isr(NULL);
	}

	if (sunxi_udc_source.dma_recv_channal < 32U && dma_irq & (1U << sunxi_udc_source.dma_recv_channal)) {
		sunxi_usb_recv_by_dma_isr(NULL);
	}

	usb_dma_int_clear();
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
	return;
}

void sunxi_usb_attach()
{
	if (sunxi_udev_active == NULL) {
		printk_error("USB: no device function attached\n");
		return;
	}

	interrupt_enable();

	for (;;) {
		sunxi_udev_active->state_loop(&sunxi_ubuf);
		sunxi_usb_process_deferred_rx();
	}
}

int sunxi_usb_extern_loop()
{
	if (sunxi_udev_active == NULL) {
		return -1;
	}

	int ret = sunxi_udev_active->state_loop(&sunxi_ubuf);
	sunxi_usb_process_deferred_rx();
	return ret;
}

void sunxi_usb_bulk_ep_reset()
{
	uint8_t old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
	/* tx */
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_IN_EP_INDEX);
	usb_device_config_ep(
		sunxi_udc_source.usbc_hd, USBC_TS_TYPE_BULK, USBC_EP_TYPE_TX, 1, sunxi_udc_source.bulk_ep_max & 0x7ff);
	usb_controller_config_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, 1, sunxi_udc_source.fifo_size,
		(uint32_t)sunxi_udc_source.bulk_out_addr);
	usb_controller_int_enable_ep(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_BULK_IN_EP_INDEX);

	/* rx */
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);
	usb_device_config_ep(
		sunxi_udc_source.usbc_hd, USBC_TS_TYPE_BULK, USBC_EP_TYPE_RX, 1, sunxi_udc_source.bulk_ep_max & 0x7ff);
	usb_controller_config_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1, sunxi_udc_source.fifo_size,
		(uint32_t)sunxi_udc_source.bulk_in_addr);
	usb_controller_int_enable_ep(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, SUNXI_USB_BULK_OUT_EP_INDEX);

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
	return;
}

int sunxi_usb_start_recv_by_dma(void *mem_base, uint32_t length)
{
	uint32_t old_ep_idx;
	uint32_t mem_buf;
	uint32_t dma_length;
	uintptr_t mem_addr;
	int ret;

	if (mem_base == NULL || length == 0U || sunxi_udc_source.usbc_hd == 0U ||
		sunxi_udc_source.dma_recv_channal == 0U || usb_dma_rx_active)
		return -1;

	mem_addr = (uintptr_t)mem_base;
	if (mem_addr > 0xffffffffU || (uint32_t)mem_addr > 0xffffffffU - length)
		return -1;

	mem_buf = (uint32_t)mem_addr;
	dma_length = length & ~(sizeof(uint32_t) - 1U);
	if (dma_length == 0U)
		return -1;

	usb_dma_trans_unaliged_bytes = length - dma_length;
	usb_dma_trans_unaligned_buf = (uint8_t *)mem_base + dma_length;
	usb_dma_rx_error = 0;
	usb_dma_rx_active = 1;

	old_ep_idx = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);

	ret = usb_device_config_ep_dma(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
	if (ret != 0)
		goto fail;

	sunxi_ubuf.request_size = dma_length;
	printk_trace("USB: dma start 0x%x, length 0x%x\n", mem_buf, dma_length);
	ret = usb_dma_start(sunxi_udc_source.dma_recv_channal, mem_buf, dma_length);
	if (ret != 0)
		goto fail;

	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
	return 0;

fail:
	usb_dma_stop(sunxi_udc_source.dma_recv_channal);
	usb_device_clear_ep_dma(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
	usb_dma_rx_active = 0;
	usb_dma_trans_unaliged_bytes = 0;
	usb_dma_trans_unaligned_buf = NULL;
	sunxi_ubuf.request_size = 0;
	usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
	return ret;
}

int sunxi_usb_get_dma_rx_status(void)
{
	return usb_dma_rx_error ? -1 : 0;
}

int sunxi_usb_send_setup(uint32_t length, const void *buffer)
{
	uint32_t fifo = 0;

	if (length > USBC_EP0_FIFOSIZE || (length != 0U && buffer == NULL) || sunxi_udc_source.usbc_hd == 0U)
		return -1;

	if (!length) {
		/* sent zero packet */
		return sunxi_usb_write_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0, 1);
	}

	fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_CTRL_EP_INDEX);
	if (usb_controller_write_packet(sunxi_udc_source.usbc_hd, fifo, length, buffer) != length)
		return -1;

	return sunxi_usb_write_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0, 1);
}

int sunxi_usb_set_address(uint32_t address)
{
	if (address > 0x7fU)
		return SUNXI_USB_REQ_OP_ERR;

	sunxi_udc_source.address = address;
	if (usb_device_read_data_status(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0, 1) != 0)
		return SUNXI_USB_REQ_OP_ERR;
	return SUNXI_USB_REQ_SUCCESSED;
}

int sunxi_usb_send_data(void *buffer, uint32_t buffer_size)
{
	if (buffer_size != 0U && buffer == NULL)
		return -1;

	sunxi_ubuf.rx_ready_for_data = 0;
	return sunxi_usb_write_fifo((uint8_t *)buffer, buffer_size);
}

int sunxi_usb_get_ep_max(void)
{
	return sunxi_udc_source.bulk_ep_max;
}

int sunxi_usb_get_ep_in_type(void)
{
	return (0x80 | SUNXI_USB_BULK_IN_EP_INDEX);
}

int sunxi_usb_get_ep_out_type(void)
{
	return SUNXI_USB_BULK_OUT_EP_INDEX;
}

void sunxi_usb_dump(uint32_t usbc_base, uint32_t ep_index)
{
	uint32_t old_ep_index = 0;

	old_ep_index = readw(usbc_base + USBC_REG_o_EPIND);
	writew(ep_index, (usbc_base + USBC_REG_o_EPIND));
	printk_trace("old_ep_index = %d, ep_index = %d\n", old_ep_index, ep_index);

	printk_trace("USBC_REG_o_FADDR         = 0x%08x\n", readb(usbc_base + USBC_REG_o_FADDR));
	printk_trace("USBC_REG_o_PCTL          = 0x%08x\n", readb(usbc_base + USBC_REG_o_PCTL));
	printk_trace("USBC_REG_o_INTTx         = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTTx));
	printk_trace("USBC_REG_o_INTRx         = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTRx));
	printk_trace("USBC_REG_o_INTTxE        = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTTxE));
	printk_trace("USBC_REG_o_INTRxE        = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTRxE));
	printk_trace("USBC_REG_o_INTUSB        = 0x%08x\n", readb(usbc_base + USBC_REG_o_INTUSB));
	printk_trace("USBC_REG_o_INTUSBE       = 0x%08x\n", readb(usbc_base + USBC_REG_o_INTUSBE));
	printk_trace("USBC_REG_o_EPIND         = 0x%08x\n", readw(usbc_base + USBC_REG_o_EPIND));
	printk_trace("USBC_REG_o_TXMAXP        = 0x%08x\n", readw(usbc_base + USBC_REG_o_TXMAXP));
	printk_trace("USBC_REG_o_CSR0          = 0x%08x\n", readw(usbc_base + USBC_REG_o_CSR0));
	printk_trace("USBC_REG_o_TXCSR         = 0x%08x\n", readw(usbc_base + USBC_REG_o_TXCSR));
	printk_trace("USBC_REG_o_RXMAXP        = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXMAXP));
	printk_trace("USBC_REG_o_RXCSR         = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXCSR));
	printk_trace("USBC_REG_o_COUNT0        = 0x%08x\n", readw(usbc_base + USBC_REG_o_COUNT0));
	printk_trace("USBC_REG_o_RXCOUNT       = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXCOUNT));
	printk_trace("USBC_REG_o_TXTYPE        = 0x%08x\n", readb(usbc_base + USBC_REG_o_TXTYPE));
	printk_trace("USBC_REG_o_NAKLIMIT0     = 0x%08x\n", readb(usbc_base + USBC_REG_o_NAKLIMIT0));
	printk_trace("USBC_REG_o_TXINTERVAL    = 0x%08x\n", readb(usbc_base + USBC_REG_o_TXINTERVAL));
	printk_trace("USBC_REG_o_RXTYPE        = 0x%08x\n", readb(usbc_base + USBC_REG_o_RXTYPE));
	printk_trace("USBC_REG_o_RXINTERVAL    = 0x%08x\n", readb(usbc_base + USBC_REG_o_RXINTERVAL));
	printk_trace("USBC_REG_o_CONFIGDATA    = 0x%08x\n", readb(usbc_base + USBC_REG_o_CONFIGDATA));

	printk_trace("USBC_REG_o_DEVCTL        = 0x%08x\n", readb(usbc_base + USBC_REG_o_DEVCTL));
	printk_trace("USBC_REG_o_TXFIFOSZ      = 0x%08x\n", readb(usbc_base + USBC_REG_o_TXFIFOSZ));
	printk_trace("USBC_REG_o_RXFIFOSZ      = 0x%08x\n", readb(usbc_base + USBC_REG_o_RXFIFOSZ));
	printk_trace("USBC_REG_o_TXFIFOAD      = 0x%08x\n", readw(usbc_base + USBC_REG_o_TXFIFOAD));
	printk_trace("USBC_REG_o_RXFIFOAD      = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXFIFOAD));
	printk_trace("USBC_REG_o_VEND0         = 0x%08x\n", readb(usbc_base + USBC_REG_o_VEND0));
	printk_trace("USBC_REG_o_VEND1         = 0x%08x\n", readb(usbc_base + USBC_REG_o_VEND1));

	printk_trace("=====================================\n");
	printk_trace("TXFADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_TXFADDRx));
	printk_trace("TXHADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_TXHADDRx));
	printk_trace("TXHPORTx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_TXHPORTx));
	printk_trace("RXFADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_RXFADDRx));
	printk_trace("RXHADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_RXHADDRx));
	printk_trace("RXHPORTx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_RXHPORTx));
	printk_trace("RPCOUNTx(%d)              = 0x%08x\n", ep_index, (uint32_t)readw(usbc_base + USBC_REG_o_RPCOUNT));
	printk_trace("=====================================\n");

	printk_trace("USBC_REG_o_ISCR          = 0x%08x\n", (uint32_t)readl(usbc_base + USBC_REG_o_ISCR));
	printk_trace("USBC_REG_o_PHYCTL        = 0x%08x\n", (uint32_t)readl(usbc_base + USBC_REG_o_PHYCTL));
	printk_trace("USBC_REG_o_PHYBIST       = 0x%08x\n", (uint32_t)readl(usbc_base + USBC_REG_o_PHYBIST));

	writew(old_ep_index, (usbc_base + USBC_REG_o_EPIND));

	return;
}
