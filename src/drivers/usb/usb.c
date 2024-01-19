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
#include "usb_controller.h"

#define RX_BUFF_SIZE (512)

static sunxi_udc_t sunxi_udc_source;
static sunxi_ubuf_t sunxi_ubuf;
static sunxi_usb_setup_req_t *sunxi_udev_active;

void sunxi_usb_attach(uint32_t device_type) {
    sunxi_usb_module_reg(SUNXI_USB_DEVICE_DETECT);
    switch (device_type) {
        case SUNXI_USB_DEVICE_DETECT:
            sunxi_usb_module_reg(SUNXI_USB_DEVICE_DETECT);
            break;
        default:
            printk(LOG_LEVEL_ERROR, "USB: unknown device, type id = %d\n", device_type);
            break;
    }
}

void sunxi_usb_init() {
    uint32_t reg_val = 0;
    static uint8_t rx_base_buffer[RX_BUFF_SIZE];

    if (sunxi_udev_active->state_init()) {
        printk(LOG_LEVEL_ERROR, "USB: fail to init usb device\n");
        return;
    }
    memset(&sunxi_udc_source, 0, sizeof(sunxi_udc_t));
    sunxi_udc_source.usbc_hd = usb_controller_open_otg(0);
    if (sunxi_udc_source.usbc_hd == 0) {
        printk(LOG_LEVEL_ERROR, "USB: usb_controller_open_otg failed\n");
        return;
    }

    sunxi_usb_dma_init(sunxi_udc_source.usbc_hd);
    

}