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

uint32_t usb_controller_open_otg(uint32_t otg_no) {
    usb_controller_otg_t *usbc_otg = usbc_otg_array;

    if (otg_no >= USBC_MAX_CTL_NUM) {
        return 0;
    }

    usbc_otg[otg_no].used = 1;
    usbc_otg[otg_no].no = otg_no;
    usbc_otg[otg_no].port_num = otg_no;
    usbc_otg[otg_no].base_addr = usbc_base_address[otg_no];

    return (uint32_t) (&(usbc_otg[otg_no]));
}

int usb_controller_close_otg(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    if (usbc_otg == NULL) {
        return -1;
    }
    memset(usbc_otg, 0, sizeof(usb_controller_otg_t));
    return 0;
}

static uint32_t usb_controller_wake_up_clear_change_detect(uint32_t reg_val) {
    uint32_t temp = reg_val;
    temp &= ~(1 << USBC_BP_ISCR_VBUS_CHANGE_DETECT);
    temp &= ~(1 << USBC_BP_ISCR_ID_CHANGE_DETECT);
    temp &= ~(1 << USBC_BP_ISCR_DPDM_CHANGE_DETECT);
    return temp;
}

static void usb_controller_force_id_low(uint32_t addr) {
    uint32_t reg_val = 0;

    /* vbus, id, dpdm变化位是写1清零, 因此我们在操作其他bit的时候清除这些位 */
    reg_val = read32(USBC_REG_ISCR(addr));
    reg_val &= ~(0x03 << USBC_BP_ISCR_FORCE_ID);
    reg_val |= (0x02 << USBC_BP_ISCR_FORCE_ID);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(addr));
}

static void usb_controller_force_id_high(uint32_t addr) {
    uint32_t reg_val = 0;

    /*先写00，后写10*/
    reg_val = read32(USBC_REG_ISCR(addr));
    reg_val |= (0x03 << USBC_BP_ISCR_FORCE_ID);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(addr));
}

static void usb_controller_force_id_disable(uint32_t addr) {
    uint32_t reg_val = 0;

    /*vbus, id, dpdm变化位是写1清零, 因此我们在操作其他bit的时候清除这些位*/
    reg_val = read32(USBC_REG_ISCR(addr));
    reg_val &= ~(0x03 << USBC_BP_ISCR_FORCE_ID);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(addr));
}

void usb_controller_force_id_status(uint32_t husb, uint32_t id_type) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    switch (id_type) {
        case USBC_ID_TYPE_HOST:
            usb_controller_force_id_low(usbc_otg->base_addr);
            break;
        case USBC_ID_TYPE_DEVICE:
            usb_controller_force_id_high(usbc_otg->base_addr);
            break;
        default:
            usb_controller_force_id_disable(usbc_otg->base_addr);
            break;
    }
}

static void usb_controller_force_vbus_valid_disable(uint32_t addr) {
    uint32_t reg_val = 0;

    /*先写00，后写10*/
    reg_val = read32(USBC_REG_ISCR(addr));
    reg_val &= ~(0x03 << USBC_BP_ISCR_FORCE_VBUS_VALID);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(addr));
}

static void usb_controller_force_vbus_valid_low(uint32_t addr) {
    uint32_t reg_val = 0;

    /*先写00，后写10*/
    reg_val = read32(USBC_REG_ISCR(addr));
    reg_val &= ~(0x03 << USBC_BP_ISCR_FORCE_VBUS_VALID);
    reg_val |= (0x02 << USBC_BP_ISCR_FORCE_VBUS_VALID);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(addr));
}

static void usb_controller_force_vbus_valid_high(uint32_t addr) {
    uint32_t reg_val = 0;

    reg_val = read32(USBC_REG_ISCR(addr));
    reg_val |= (0x03 << USBC_BP_ISCR_FORCE_VBUS_VALID);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(addr));
}

/* force vbus valid to (vbus_type) */
void usb_controller_force_vbus_valid(uint32_t husb, uint32_t vbus_type) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    switch (vbus_type) {
        case USBC_VBUS_TYPE_LOW:
            usb_controller_force_vbus_valid_low(usbc_otg->base_addr);
            break;
        case USBC_VBUS_TYPE_HIGH:
            usb_controller_force_vbus_valid_high(usbc_otg->base_addr);
            break;
        default:
            usb_controller_force_vbus_valid_disable(usbc_otg->base_addr);
            break;
    }
}

void usb_controller_id_pull_enable(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;

    /*vbus, id, dpdm变化位是写1清零, 因此我们在操作其他bit的时候清除这些位*/
    reg_val = read32(USBC_REG_ISCR(usbc_otg->base_addr));
    reg_val |= (1 << USBC_BP_ISCR_ID_PULLUP_EN);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(usbc_otg->base_addr));
}

void usb_controller_id_pull_disable(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;

    /*vbus, id, dpdm变化位是写1清零, 因此我们在操作其他bit的时候清除这些位*/
    reg_val = read32(USBC_REG_ISCR(usbc_otg->base_addr));
    reg_val &= ~(1 << USBC_BP_ISCR_ID_PULLUP_EN);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(usbc_otg->base_addr));
}

void usb_controller_dpdm_pull_enable(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;

    /*vbus, id, dpdm变化位是写1清零, 因此我们在操作其他bit的时候清除这些位*/
    reg_val = read32(USBC_REG_ISCR(usbc_otg->base_addr));
    reg_val |= (1 << USBC_BP_ISCR_DPDM_PULLUP_EN);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(usbc_otg->base_addr));
}

void usb_controller_dpdm_pull_disable(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;

    /*vbus, id, dpdm变化位是写1清零, 因此我们在操作其他bit的时候清除这些位*/
    reg_val = read32(USBC_REG_ISCR(usbc_otg->base_addr));
    reg_val &= ~(1 << USBC_BP_ISCR_DPDM_PULLUP_EN);
    reg_val = usb_controller_wake_up_clear_change_detect(reg_val);
    writel(reg_val, USBC_REG_ISCR(usbc_otg->base_addr));
}

void usb_controller_select_bus(uint32_t husb, uint32_t io_type, uint32_t ep_type, uint32_t ep_index) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;
    if (usbc_otg == NULL) {
        return;
    }
    reg_val = read8(USBC_REG_VEND0(usbc_otg->base_addr));
    if (io_type == USBC_IO_TYPE_DMA) {
        if (ep_type == USBC_EP_TYPE_TX) {
            reg_val |= ((ep_index - 0x01) << 1) << USBC_BP_VEND0_DRQ_SEL; /*drq_sel*/
            reg_val |= 0x1 << USBC_BP_VEND0_BUS_SEL;                      /*io_dma*/
        } else {
            reg_val |= ((ep_index << 1) - 0x01) << USBC_BP_VEND0_DRQ_SEL;
            reg_val |= 0x1 << USBC_BP_VEND0_BUS_SEL;
        }
    } else {
        reg_val &= 0x00; /*清除drq_sel, 选择pio*/
    }
    writeb(reg_val, USBC_REG_VEND0(usbc_otg->base_addr));
}

void usb_controller_config_fifo_base(uint32_t husb, uint32_t sram_base) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    fifo_info_t *usbc_info = &usbc_info_g;
    if (usbc_otg == NULL) {
        return;
    }
    if (usbc_otg->port_num == 0) {
        usbc_info->port0_fifo_addr = 0x00;
        usbc_info->port0_fifo_size = (8 * 1024);
    }
}

void usb_controller_int_disable_usb_misc_all(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return;
    }

    writeb(0, USBC_REG_INTUSBE(usbc_otg->base_addr));
}

void usb_controller_int_disable_ep_all(uint32_t husb, uint32_t ep_type) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return;
    }

    switch (ep_type) {
        case USBC_EP_TYPE_TX:
            usb_controller_int_disable_tx_all(usbc_otg->base_addr);
            break;

        case USBC_EP_TYPE_RX:
            usb_controller_int_disable_rx_all(usbc_otg->base_addr);
            break;

        default:
            break;
    }

    return;
}

void usb_controller_int_enable_usb_misc_unit(uint32_t husb, uint32_t mask) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;
    uint32_t reg_val = 0;

    if (usbc_otg == NULL) {
        return;
    }

    reg_val = readb(USBC_REG_INTUSBE(usbc_otg->base_addr));
    reg_val |= mask;
    writeb(reg_val, USBC_REG_INTUSBE(usbc_otg->base_addr));
}

void usb_controller_int_enable_ep(uint32_t husb, uint32_t ep_type, uint32_t ep_index) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return;
    }

    switch (ep_type) {
        case USBC_EP_TYPE_TX:
            usb_controller_int_enable_tx_ep(usbc_otg->base_addr, ep_index);
            break;

        case USBC_EP_TYPE_RX:
            usb_controller_int_enable_rx_ep(usbc_otg->base_addr, ep_index);
            break;

        default:
            break;
    }

    return;
}

uint32_t usb_controller_get_active_ep(uint32_t husb) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return 0;
    }

    return readb(USBC_REG_EPIND(usbc_otg->base_addr));
}

void usb_controller_select_active_ep(uint32_t husb, uint8_t ep_index) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return;
    }

    writeb(ep_index, USBC_REG_EPIND(usbc_otg->base_addr));
}

void usb_controller_config_fifo_tx_ep_default(uint32_t addr) {
    writew(0x00, USBC_REG_TXFIFOAD(addr));
    writeb(0x00, USBC_REG_TXFIFOSZ(addr));
}

void usb_controller_config_fifo_tx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr) {
    uint32_t temp = 0;
    uint32_t size = 0; /*fifo_size = (size + 3)的2次方*/
    uint32_t addr_base = 0; /*fifo_addr = addr * 8*/

    /*--<1>--换算sz, 不满512，以512对齐*/
    temp = fifo_size + 511;
    temp &= ~511; /*把511后面的清零*/
    temp >>= 3;
    temp >>= 1;
    while (temp) {
        size++;
        temp >>= 1;
    }
    addr_base = fifo_addr >> 3;
    writew(addr_base, USBC_REG_TXFIFOAD(addr));

    /* config fifo size */
    writeb((size & 0x0f), USBC_REG_TXFIFOSZ(addr));
    if (is_double_fifo) {
        usb_set_bit8(USBC_BP_TXFIFOSZ_DPB, USBC_REG_TXFIFOSZ(addr));
    }
}

void usb_controller_config_fifo_rx_ep_default(uint32_t addr) {
    writew(0x00, USBC_REG_RXFIFOAD(addr));
    writeb(0x00, USBC_REG_RXFIFOSZ(addr));
}

void usb_controller_config_fifo_rx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr) {
    uint32_t temp = 0;
    uint32_t size = 0; /*fifo_size = (size + 3)的2次方*/
    uint32_t addr_base = 0; /*fifo_addr = addr * 8*/

    /*--<1>--换算sz, 不满512，以512对齐*/
    temp = fifo_size + 511;
    temp &= ~511; /*把511后面的清零*/
    temp >>= 3;
    temp >>= 1;
    while (temp) {
        size++;
        temp >>= 1;
    }
    addr_base = fifo_addr >> 3;
    writew(addr_base, USBC_REG_RXFIFOAD(addr));
    writeb((size & 0x0f), USBC_REG_RXFIFOSZ(addr));
    if (is_double_fifo) {
        usb_set_bit8(USBC_BP_RXFIFOSZ_DPB, USBC_REG_RXFIFOSZ(addr));
    }
}

void usb_controller_config_fifo(uint32_t husb, uint32_t ep_type, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr) {
    usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

    if (usbc_otg == NULL) {
        return;
    }

    switch (ep_type) {
        case USBC_EP_TYPE_EP0:
            break;

        case USBC_EP_TYPE_TX:
            usb_controller_config_fifo_tx_ep(usbc_otg->base_addr, is_double_fifo, fifo_size, fifo_addr);
            break;

        case USBC_EP_TYPE_RX:
            usb_controller_config_fifo_rx_ep(usbc_otg->base_addr, is_double_fifo, fifo_size, fifo_addr);
            break;

        default:
            break;
    }
}
