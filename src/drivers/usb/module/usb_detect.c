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

#include "usb.h"

volatile uint32_t sunxi_usb_detect_flag;

static int sunxi_usb_detect_init(void) {
    printk(LOG_LEVEL_DEBUG, "USB: sunxi_usb_detect_init\n");
    sunxi_usb_detect_flag = 0;
    return 0;
}

static int sunxi_usb_detect_exit(void) {
    printk(LOG_LEVEL_DEBUG, "USB: sunxi_usb_detect_exit\n");
    sunxi_usb_detect_flag = 0;
    return 0;
}

static void sunxi_usb_detect_reset(void) {
}

static void sunxi_usb_detect_usb_rx_dma_isr(void *p_arg) {
    printk(LOG_LEVEL_DEBUG, "USB: dma int for usb rx occur\n");
}

static void sunxi_usb_detect_usb_tx_dma_isr(void *p_arg) {
    printk(LOG_LEVEL_DEBUG, "USB: dma int for usb tx occur\n");
}

static int sunxi_usb_detect_standard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer) {
    return 0;
}

static int sunxi_usb_detect_nonstandard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status) {
    return 0;
}

static int sunxi_usb_detect_state_loop(void *buffer) {
    return 0;
}

sunxi_usb_module_init(SUNXI_USB_DEVICE_DETECT, sunxi_usb_detect_init, sunxi_usb_detect_exit, sunxi_usb_detect_reset,
                      sunxi_usb_detect_standard_req_op, sunxi_usb_detect_nonstandard_req_op, sunxi_usb_detect_state_loop,
                      sunxi_usb_detect_usb_rx_dma_isr, sunxi_usb_detect_usb_tx_dma_isr);
