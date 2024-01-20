/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_CONTROLLER_H__
#define __USB_CONTROLLER_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-usb.h"

#define USBC_MAX_OPEN_NUM 8
#define USBC_MAX_CTL_NUM 3
#define USBC_MAX_EP_NUM 6
#define USBC0_MAX_FIFO_SIZE (8 * 1024)
#define USBC_EP0_FIFOSIZE 64

typedef struct fifo_info {
    uint32_t port0_fifo_addr;
    uint32_t port0_fifo_size;
} fifo_info_t;

/* Structure to store hardware information for the current USB port */
typedef struct usb_controller_otg {
    uint32_t port_num;       /* USB port number */
    uint32_t base_addr;      /* USB base address */

    uint32_t used;           /* Whether it is currently being used */
    uint32_t no;             /* Position in the management array */
} usb_controller_otg_t;

/* Get the interrupt pending flag of a TX endpoint */
static inline uint32_t usb_controller_int_tx_pending(uint32_t addr) {
    return readw(USBC_REG_INTTx(addr));
}

/* Clear the interrupt pending flag of a TX endpoint */
static inline void usb_controller_int_clear_tx_pending(uint32_t addr, uint8_t ep_index) {
    writew((1 << ep_index), USBC_REG_INTTx(addr));
}

/* Clear the interrupt pending flags of all TX endpoints */
static inline void usb_controller_int_clear_tx_pending_all(uint32_t addr) {
    writew(0xffff, USBC_REG_INTTx(addr));
}

/* Get the interrupt pending flag of an RX endpoint */
static inline uint32_t usb_controller_int_rx_pending(uint32_t addr) {
    return readw(USBC_REG_INTRx(addr));
}

/* Clear the interrupt pending flag of an RX endpoint */
static inline void usb_controller_int_clear_rx_pending(uint32_t addr, uint8_t ep_index) {
    writew((1 << ep_index), USBC_REG_INTRx(addr));
}

/* Clear the interrupt pending flags of all RX endpoints */
static inline void usb_controller_int_clear_rx_pending_all(uint32_t addr) {
    writew(0xffff, USBC_REG_INTRx(addr));
}

/* Enable the interrupt of a TX endpoint */
static inline void usb_controller_int_enable_tx_ep(uint32_t addr, uint8_t ep_index) {
    usb_set_bit16(ep_index, USBC_REG_INTTxE(addr));
}

/* Enable the interrupt of an RX endpoint */
static inline void usb_controller_int_enable_rx_ep(uint32_t addr, uint8_t ep_index) {
    usb_set_bit16(ep_index, USBC_REG_INTRxE(addr));
}

/* Disable the interrupt of a TX endpoint */
static inline void usb_controller_int_disable_tx_ep(uint32_t addr, uint8_t ep_index) {
    usb_clear_bit16(ep_index, USBC_REG_INTTxE(addr));
}

/* Disable the interrupt of an RX endpoint */
static inline void usb_controller_int_disable_rx_ep(uint32_t addr, uint8_t ep_index) {
    usb_clear_bit16(ep_index, USBC_REG_INTRxE(addr));
}

/* Disable the interrupts of all TX endpoints */
static inline void usb_controller_int_disable_tx_all(uint32_t addr) {
    writew(0, USBC_REG_INTTxE(addr));
}

/* Disable the interrupts of all RX endpoints */
static inline void usb_controller_int_disable_rx_all(uint32_t addr) {
    writew(0, USBC_REG_INTRxE(addr));
}


uint32_t usb_controller_open_otg(uint32_t otg_no);

int usb_controller_close_otg(uint32_t husb);

void usb_controller_force_id_status(uint32_t husb, uint32_t id_type);

/* force vbus valid to (vbus_type) */
void usb_controller_force_vbus_valid(uint32_t husb, uint32_t vbus_type);

void usb_controller_id_pull_enable(uint32_t husb);

void usb_controller_id_pull_disable(uint32_t husb);

void usb_controller_dpdm_pull_enable(uint32_t husb);

void usb_controller_dpdm_pull_disable(uint32_t husb);

void usb_controller_config_fifo_base(uint32_t husb, uint32_t sram_base);

void usb_controller_int_disable_usb_misc_all(uint32_t husb);

void usb_controller_int_disable_ep_all(uint32_t husb, uint32_t ep_type);

void usb_controller_int_enable_usb_misc_unit(uint32_t husb, uint32_t mask);

uint32_t usb_controller_get_active_ep(uint32_t husb);

void usb_controller_select_active_ep(uint32_t husb, uint8_t ep_index);

void usb_controller_config_fifo_tx_ep_default(uint32_t addr);

void usb_controller_config_fifo_tx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

void usb_controller_config_fifo_rx_ep_default(uint32_t addr);

void usb_controller_config_fifo_rx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

void usb_controller_config_fifo(uint32_t husb, uint32_t ep_type, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

void usb_controller_int_enable_ep(uint32_t husb, uint32_t ep_type, uint32_t ep_index);

#endif// __USB_CONTROLLER_H__