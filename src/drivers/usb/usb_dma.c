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
#include "usb_dma.h"

#define SUNXI_USB_DMA_MAX (8)

static uint32_t usb_hd;
static uint32_t usb_dma_used[SUNXI_USB_DMA_MAX];

static int sunxi_usb_index_check(uint32_t dma_index) {
    if (dma_index > SUNXI_USB_DMA_MAX) {
        printk(LOG_LEVEL_ERROR, "USB: usb dma %d is overrange\n", dma_index);
        return -1;
    }
    if (!usb_dma_used[dma_index]) {
        printk(LOG_LEVEL_ERROR, "USB: usb dma %d is not used\n", dma_index);
        return -1;
    }
    return 0;
}

int sunxi_usb_dma_init(uint32_t husb) {
    usb_hd = husb;
    return 0;
}

int sunxi_usb_dma_request(void) {
    for (int i = 1; i < SUNXI_USB_DMA_MAX; i++) {
        if (usb_dma_used[i] == 0) {
            usb_dma_used[i] = 1;
            return i;
        }
    }
    return 0;
}

int sunxi_usb_dma_release(uint32_t dma_index) {
    int ret = sunxi_usb_index_check(dma_index);
    if (ret) {
        return ret;
    }
    usb_dma_used[dma_index] = 0;
    return 0;
}

int sunxi_usb_dma_setting(uint32_t dma_index, uint32_t trans_dir, uint32_t ep) {
    int ret = sunxi_usb_index_check(dma_index);
    if (ret) {
        return ret;
    }
    return usb_controller_dma_set_channal_para(usb_hd, dma_index, trans_dir, ep);
}