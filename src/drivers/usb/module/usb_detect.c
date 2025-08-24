/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <sys-clk.h>

#include "usb_defs.h"
#include "usb.h"


volatile uint32_t sunxi_usb_detect_flag;

static int sunxi_usb_detect_init(void) {
	printk_debug("USB: sunxi_usb_detect_init\n");
	sunxi_usb_detect_flag = 0;
	return 0;
}

static int sunxi_usb_detect_exit(void) {
	printk_debug("USB: sunxi_usb_detect_exit\n");
	sunxi_usb_detect_flag = 0;
	return 0;
}

static void sunxi_usb_detect_reset(void) {
}

static void sunxi_usb_detect_usb_rx_dma_isr(void *p_arg) {
	printk_debug("USB: dma int for usb rx occur\n");
}

static void sunxi_usb_detect_usb_tx_dma_isr(void *p_arg) {
	printk_debug("USB: dma int for usb tx occur\n");
}

static int sunxi_usb_detect_standard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer) {
	int ret = SUNXI_USB_REQ_OP_ERR;

	printk_trace("USB: sunxi_usb_detect_standard_req_op get cmd = %d\n", cmd);
	switch (cmd) {
		case USB_REQ_GET_STATUS: {
			break;
		}
		case USB_REQ_SET_ADDRESS: {
			break;
		}
		case USB_REQ_GET_DESCRIPTOR:
		case USB_REQ_GET_CONFIGURATION: {
			break;
		}
		case USB_REQ_SET_CONFIGURATION: {
			break;
		}
		case USB_REQ_SET_INTERFACE: {
			break;
		}
		default: {
			printk_error("usb detect error: standard req is not supported\n");
			ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
			break;
		}
	}
	return ret;
}

static int sunxi_usb_detect_nonstandard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status) {
	return 0;
}

static int sunxi_usb_detect_state_loop(void *buffer) {
	printk_trace("USB: sunxi_usb_detect_state_loop get\n");
	return 0;
}

sunxi_usb_module_init(SUNXI_USB_DEVICE_DETECT, sunxi_usb_detect_init, sunxi_usb_detect_exit, sunxi_usb_detect_reset, sunxi_usb_detect_standard_req_op,
					  sunxi_usb_detect_nonstandard_req_op, sunxi_usb_detect_state_loop, sunxi_usb_detect_usb_rx_dma_isr, sunxi_usb_detect_usb_tx_dma_isr);
