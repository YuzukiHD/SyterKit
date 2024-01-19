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

static uint32_t usbc_base_address[USBC_MAX_CTL_NUM] = {SUNXI_USB0_BASE};
static usb_controller_otg_t usbc_otg_array[USBC_MAX_OPEN_NUM];
static fifo_info_t usbc_info_g;

uint64_t usb_controller_open_otg(uint32_t otg_no) {
    usb_controller_otg_t *usbc_otg = usbc_otg_array;

    if (otg_no >= USBC_MAX_CTL_NUM) {
        return 0;
    }

    usbc_otg[otg_no].used = 1;
    usbc_otg[otg_no].no = otg_no;
    usbc_otg[otg_no].port_num = otg_no;
    usbc_otg[otg_no].base_addr = usbc_base_address[otg_no];

    return (uint64_t) (&(usbc_otg[otg_no]));
}

int usb_controller_dma_set_channal_para(uint64_t husb, uint32_t dma_chan, uint32_t trans_dir, uint32_t ep_type) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;
    if (usbc_otg == NULL) {
        return -1;
    }
    reg_val = readl(usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan);
    reg_val &= ~((1 << 4) | (0xf << 0));
    reg_val |= ((trans_dir & 1) << 4);
    reg_val |= ((ep_type & 0xf) << 0);
    writel(reg_val, usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan);
    return 0;
}

void usb_controller_dev_connect_switch(uint64_t husb, uint32_t is_on) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return;
    }
}