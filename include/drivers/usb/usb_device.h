/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-usb.h"

/* disable all transfer type */
static inline void usb_device_transfer_type_default(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(addr));
}

/* set control transfer type */
static inline void usb_device_transfer_type_ctrl(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(addr));
}

/* set ISO transfer type */
static inline void usb_device_transfer_type_iso(uint32_t addr) {
    usb_set_bit8(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(addr));
}

/* set interrupt transfer type */
static inline void usb_device_transfer_type_int(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(addr));
}

/* set bulk transfer type */
static inline void usb_device_transfer_type_bulk(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(addr));
}

/* disable all mode */
static inline void usb_device_transfer_mode_default(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(addr));
}

/* set mode to hs -> hs (fake ss) */
static inline void usb_device_transfer_mode_ss(uint32_t addr) {
    usb_set_bit8(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(addr));
}

/* set mode to hs */
static inline void usb_device_transfer_mode_hs(uint32_t addr) {
    usb_set_bit8(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(addr));
}

/* set mode to fs */
static inline void usb_device_transfer_mode_fs(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(addr));
}

/* set mode to ls -> fs (fake ls) */
static inline void usb_device_transfer_mode_ls(uint32_t addr) {
    usb_clear_bit8(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(addr));
}

/* config ep0 disable */
static inline void usb_device_ep0_config_ep0_default(uint32_t addr) {
    writew(1 << USBC_BP_CSR0_D_FLUSH_FIFO, USBC_REG_CSR0(addr));
}

/* config ep0 disable */
static inline void usb_device_ep0_config_ep0(uint32_t addr) {
    writew(1 << USBC_BP_CSR0_D_FLUSH_FIFO, USBC_REG_CSR0(addr));
}

/* ep0 get read ready flag */
static inline uint32_t usb_device_ep0_get_read_data_ready(uint32_t addr) {
    return usb_get_bit16(USBC_BP_CSR0_D_RX_PKT_READY, USBC_REG_CSR0(addr));
}

/* ep0 get write ready flag */
static inline uint32_t usb_device_ep0_get_write_data_ready(uint32_t addr) {
    return usb_get_bit16(USBC_BP_CSR0_D_TX_PKT_READY, USBC_REG_CSR0(addr));
}

/* ep0 get read half flag */
static inline void usb_device_ep0_read_data_half(uint32_t addr) {
    writew(1 << USBC_BP_CSR0_D_SERVICED_RX_PKT_READY, USBC_REG_CSR0(addr));
}

/* ep0 get read complete flag */
static inline void usb_device_ep0_read_data_complete(uint32_t addr) {
    writew((1 << USBC_BP_CSR0_D_SERVICED_RX_PKT_READY) | (1 << USBC_BP_CSR0_D_DATA_END),
           USBC_REG_CSR0(addr));
}

/* ep0 get write half flag */
static inline void usb_device_ep0_write_data_half(uint32_t addr) {
    writew(1 << USBC_BP_CSR0_D_TX_PKT_READY, USBC_REG_CSR0(addr));
}

/* ep0 get write complete flag */
static inline void usb_device_ep0_write_data_complete(uint32_t addr) {
    writew((1 << USBC_BP_CSR0_D_TX_PKT_READY) | (1 << USBC_BP_CSR0_D_DATA_END),
           USBC_REG_CSR0(addr));
}

/* ep0 get stall flag */
static uint32_t usb_device_ep0_get_stall(uint32_t addr) {
    return usb_get_bit16(USBC_BP_CSR0_D_SENT_STALL, USBC_REG_CSR0(addr));
}

/* ep0 send stall flag */
static inline void usb_device_ep0_send_stall(uint32_t addr) {
    usb_set_bit16(USBC_BP_CSR0_D_SEND_STALL, USBC_REG_CSR0(addr));
}

/* ep0 clead stall flag */
static inline void usb_device_ep0_clear_stall(uint32_t addr) {
    usb_clear_bit16(USBC_BP_CSR0_D_SEND_STALL, USBC_REG_CSR0(addr));
    usb_clear_bit16(USBC_BP_CSR0_D_SENT_STALL, USBC_REG_CSR0(addr));
}

/* ep0 get setup end flag */
static uint32_t usb_device_ep0_get_setup_end(uint32_t addr) {
    return usb_get_bit16(USBC_BP_CSR0_D_SETUP_END, USBC_REG_CSR0(addr));
}

/* ep0 clear setup end flag */
static inline void usb_device_ep0_clear_setup_end(uint32_t addr) {
    usb_set_bit16(USBC_BP_CSR0_D_SERVICED_SETUP_END, USBC_REG_CSR0(addr));
}

/* tx enable iso ep */
static inline void usb_device_tx_iso_ep_enable(uint32_t addr) {
    usb_set_bit16(USBC_BP_TXCSR_D_ISO, USBC_REG_TXCSR(addr));
}

/* tx enable int ep */
static inline void usb_device_tx_int_ep_enable(uint32_t addr) {
    usb_clear_bit16(USBC_BP_TXCSR_D_ISO, USBC_REG_TXCSR(addr));
}

/* tx enable bulk ep */
static inline void usb_device_tx_bulk_ep_enable(uint32_t addr) {
    usb_clear_bit16(USBC_BP_TXCSR_D_ISO, USBC_REG_TXCSR(addr));
}

static inline void usb_device_tx_config_ep_default(uint32_t addr) {
    /* clear tx csr */
    writew(0x00, USBC_REG_TXCSR(addr));
    /* clear tx ep max packet */
    writew(0x00, USBC_REG_TXMAXP(addr));
}

static inline void usb_device_clear_dma_trans(uint32_t addr) {
    uint32_t reg_val;

    reg_val = readl(addr + USBC_REG_o_PCTL);
    reg_val &= ~(1 << 24);
    writel(reg_val, addr + USBC_REG_o_PCTL);
}

static inline void usb_device_config_dma_trans(uint32_t addr) {
    uint32_t reg_val;

    reg_val = readl(addr + USBC_REG_o_PCTL);
    reg_val |= (1 << 24);
    writel(reg_val, addr + USBC_REG_o_PCTL);
}

static inline void usb_device_tx_config_ep(uint32_t addr, uint32_t ts_type, uint32_t is_double_fifo, uint32_t ep_maxpkt) {
    uint16_t reg_val = 0;
    uint16_t temp = 0;

    /* config tx csr */
    reg_val = (1 << USBC_BP_TXCSR_D_MODE);
    reg_val |= (1 << USBC_BP_TXCSR_D_CLEAR_DATA_TOGGLE);
    reg_val |= (1 << USBC_BP_TXCSR_D_FLUSH_FIFO);
    writew(reg_val, USBC_REG_TXCSR(addr));

    if (is_double_fifo) {
        writew(reg_val, USBC_REG_TXCSR(addr));
    }

    /* config tx ep max packet */
    reg_val = readw(USBC_REG_TXMAXP(addr));
    temp = ep_maxpkt & ((1 << USBC_BP_TXMAXP_PACKET_COUNT) - 1);
    reg_val &= ~(0x1fff);
    reg_val |= temp;
    writew(reg_val, USBC_REG_TXMAXP(addr));

    /* config ep transfer type */
    switch (ts_type) {
        case USBC_TS_TYPE_ISO:
            usb_device_tx_iso_ep_enable(addr);
            break;
        case USBC_TS_TYPE_INT:
            usb_device_tx_int_ep_enable(addr);
            break;
        case USBC_TS_TYPE_BULK:
            usb_device_tx_bulk_ep_enable(addr);
            break;
        default:
            usb_device_tx_bulk_ep_enable(addr);
            break;
    }
}

static inline void usb_device_tx_config_ep_dma(uint32_t addr) {
    uint16_t ep_csr = 0;

    /* auto_set, tx_mode, dma_tx_en, mode1 */
    ep_csr = readb(USBC_REG_TXCSR(addr) + 1);
    ep_csr |= (1 << USBC_BP_TXCSR_D_AUTOSET) >> 8;
    ep_csr |= (1 << USBC_BP_TXCSR_D_MODE) >> 8;
    ep_csr |= (1 << USBC_BP_TXCSR_D_DMA_REQ_EN) >> 8;
    ep_csr |= (1 << USBC_BP_TXCSR_D_DMA_REQ_MODE) >> 8;
    writeb(ep_csr, (USBC_REG_TXCSR(addr) + 1));
}

static inline void usb_device_tx_clear_ep_dma(uint32_t addr) {
    uint16_t ep_csr = 0;

    /*auto_set, dma_tx_en, mode1*/
    ep_csr = readb(USBC_REG_TXCSR(addr) + 1);
    ep_csr &= ~((1 << USBC_BP_TXCSR_D_AUTOSET) >> 8);
    ep_csr &= ~((1 << USBC_BP_TXCSR_D_DMA_REQ_EN) >> 8);
    writeb(ep_csr, (USBC_REG_TXCSR(addr) + 1));

    /* DMA_REQ_EN和DMA_REQ_MODE不能在同一个cycle中清除 */
    ep_csr = readb(USBC_REG_TXCSR(addr) + 1);
    ep_csr &= ~((1 << USBC_BP_TXCSR_D_DMA_REQ_MODE) >> 8);
    writeb(ep_csr, (USBC_REG_TXCSR(addr) + 1));
}

static inline uint32_t usb_device_tx_get_write_data_ready(uint32_t addr) {
    uint32_t temp = 0;

    temp = readw(USBC_REG_TXCSR(addr));
    temp &= (1 << USBC_BP_TXCSR_D_TX_READY) |
            (1 << USBC_BP_TXCSR_D_FIFO_NOT_EMPTY);

    return temp;
}

static inline uint32_t usb_device_tx_get_write_data_ready_fifo_empty(uint32_t addr) {
    uint32_t temp = 0;

    temp = readw(USBC_REG_TXCSR(addr));
    temp &= (1 << USBC_BP_TXCSR_D_TX_READY) |
            (1 << USBC_BP_TXCSR_D_FIFO_NOT_EMPTY);

    return temp;
}

static inline void usb_device_tx_write_data_half(uint32_t addr) {
    uint16_t ep_csr = 0;

    ep_csr = readw(USBC_REG_TXCSR(addr));
    ep_csr |= 1 << USBC_BP_TXCSR_D_TX_READY;
    ep_csr &= ~(1 << USBC_BP_TXCSR_D_UNDER_RUN);
    writew(ep_csr, USBC_REG_TXCSR(addr));
}

static inline void usb_device_tx_write_data_complete(uint32_t addr) {
    uint16_t ep_csr = 0;

    ep_csr = readw(USBC_REG_TXCSR(addr));
    ep_csr |= 1 << USBC_BP_TXCSR_D_TX_READY;
    ep_csr &= ~(1 << USBC_BP_TXCSR_D_UNDER_RUN);
    writew(ep_csr, USBC_REG_TXCSR(addr));
}

/* send stall, and fifo is flushed automaticly */
static inline void usb_device_tx_send_stall(uint32_t addr) {
    usb_set_bit16(USBC_BP_TXCSR_D_SEND_STALL, USBC_REG_TXCSR(addr));
}

static inline uint32_t usb_device_tx_get_ep_stall(uint32_t addr) {
    return usb_get_bit16(USBC_BP_TXCSR_D_SENT_STALL, USBC_REG_TXCSR(addr));
}

static inline void usb_device_tx_clear_stall(uint32_t addr) {
    uint32_t reg_val;

    reg_val = readw(USBC_REG_TXCSR(addr));
    reg_val &= ~((1 << USBC_BP_TXCSR_D_SENT_STALL) |
                 (1 << USBC_BP_TXCSR_D_SEND_STALL));
    writew(reg_val, USBC_REG_TXCSR(addr));
}

static inline void usb_device_rx_enable_iso_ep(uint32_t addr) {
    usb_set_bit16(USBC_BP_RXCSR_D_ISO, USBC_REG_RXCSR(addr));
}

static inline void usb_device_rx_enable_int_ep(uint32_t addr) {
    usb_clear_bit16(USBC_BP_RXCSR_D_ISO, USBC_REG_RXCSR(addr));
}

static inline void usb_device_rx_enable_bulk_ep(uint32_t addr) {
    usb_clear_bit16(USBC_BP_RXCSR_D_ISO, USBC_REG_RXCSR(addr));
}

static inline void usb_device_rx_config_ep_default(uint32_t addr) {
    /* clear tx csr */
    writew(0x00, USBC_REG_RXCSR(addr));

    /* clear tx ep max packet */
    writew(0x00, USBC_REG_RXMAXP(addr));
}

static inline void usb_device_rx_config_ep(uint32_t addr, uint32_t ts_type, uint32_t is_double_fifo, uint32_t ep_maxpkt) {
    uint16_t reg_val = 0;
    uint16_t temp = 0;

    /* config tx csr */
    writew((1 << USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_RXCSR_D_FLUSH_FIFO),
           USBC_REG_RXCSR(addr));

    if (is_double_fifo) {
        writew((1 << USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_RXCSR_D_FLUSH_FIFO),
               USBC_REG_RXCSR(addr));
    }

    /* config tx ep max packet */
    reg_val = readw(USBC_REG_RXMAXP(addr));
    temp = ep_maxpkt & ((1 << USBC_BP_RXMAXP_PACKET_COUNT) - 1);
    reg_val &= ~(0x1fff); /*added by jerry*/
    reg_val |= temp;
    writew(reg_val, USBC_REG_RXMAXP(addr));

    /* config ep transfer type */
    switch (ts_type) {
        case USBC_TS_TYPE_ISO:
            usb_device_rx_enable_iso_ep(addr);
            break;
        case USBC_TS_TYPE_INT:
            usb_device_rx_enable_int_ep(addr);
            break;
        case USBC_TS_TYPE_BULK:
            usb_device_rx_enable_bulk_ep(addr);
            break;
        default:
            usb_device_rx_enable_bulk_ep(addr);
            break;
    }
}

static inline void usb_device_rx_config_ep_dma(uint32_t addr) {
    uint16_t ep_csr = 0;

    /* auto_clear, dma_rx_en, mode0 */
    ep_csr = readb(USBC_REG_RXCSR(addr) + 1);
    ep_csr |= ((1 << USBC_BP_RXCSR_D_AUTO_CLEAR) >> 8);
    ep_csr &= ~((1 << USBC_BP_RXCSR_D_DMA_REQ_MODE) >> 8);
    ep_csr |= ((1 << USBC_BP_RXCSR_D_DMA_REQ_EN) >> 8);
    writeb(ep_csr, (USBC_REG_RXCSR(addr) + 1));
}

static inline void usb_device_rx_clear_ep_dma(uint32_t addr) {
    uint16_t ep_csr = 0;

    /*auto_clear, dma_rx_en, mode0*/
    ep_csr = readb(USBC_REG_RXCSR(addr) + 1);
    ep_csr &= ~((1 << USBC_BP_RXCSR_D_AUTO_CLEAR) >> 8);
    ep_csr &= ~((1 << USBC_BP_RXCSR_D_DMA_REQ_MODE) >> 8);
    ep_csr &= ~((1 << USBC_BP_RXCSR_D_DMA_REQ_EN) >> 8);
    writeb(ep_csr, (USBC_REG_RXCSR(addr) + 1));
}

static inline uint32_t usb_device_rx_get_read_data_ready(uint32_t addr) {
    return usb_get_bit16(USBC_BP_RXCSR_D_RX_PKT_READY, USBC_REG_RXCSR(addr));
}

static inline void usb_device_rx_read_data_half(uint32_t addr) {
    uint32_t reg_val = 0;

    /*overrun, dataerr is used in iso transfer*/
    reg_val = readw(USBC_REG_RXCSR(addr));
    reg_val &= ~(1 << USBC_BP_RXCSR_D_RX_PKT_READY);
    reg_val &= ~(1 << USBC_BP_RXCSR_D_OVERRUN);
    reg_val &= ~(1 << USBC_BP_RXCSR_D_DATA_ERROR);
    writew(reg_val, USBC_REG_RXCSR(addr));
}

static inline void usb_device_rx_read_data_complete(uint32_t addr) {
    uint32_t reg_val = 0;

    /*overrun, dataerr is used in iso transfer*/
    reg_val = readw(USBC_REG_RXCSR(addr));
    reg_val &= ~(1 << USBC_BP_RXCSR_D_RX_PKT_READY);
    reg_val &= ~(1 << USBC_BP_RXCSR_D_OVERRUN);
    reg_val &= ~(1 << USBC_BP_RXCSR_D_DATA_ERROR);
    writew(reg_val, USBC_REG_RXCSR(addr));
}

static inline int usb_device_write_data_half(uint32_t addr, uint32_t ep_type) {
    switch (ep_type) {
        case USBC_EP_TYPE_EP0:
            usb_device_ep0_write_data_half(addr);
            break;
        case USBC_EP_TYPE_TX:
            usb_device_tx_write_data_half(addr);
            break;
        case USBC_EP_TYPE_RX:
            return -1;
        default:
            return -1;
    }

    return 0;
}

static inline int usb_device_write_data_complete(uint32_t addr, uint32_t ep_type) {
    switch (ep_type) {
        case USBC_EP_TYPE_EP0:
            usb_device_ep0_write_data_complete(addr);
            break;

        case USBC_EP_TYPE_TX:
            usb_device_tx_write_data_complete(addr);
            break;

        case USBC_EP_TYPE_RX:
            return -1;

        default:
            return -1;
    }

    return 0;
}

static inline int usb_device_read_data_half(uint32_t addr, uint32_t ep_type) {
    switch (ep_type) {
        case USBC_EP_TYPE_EP0:
            usb_device_ep0_read_data_half(addr);
            break;

        case USBC_EP_TYPE_TX:
            return -1;

        case USBC_EP_TYPE_RX:
            usb_device_rx_read_data_half(addr);
            break;

        default:
            return -1;
    }

    return 0;
}

static inline int usb_device_read_data_complete(uint32_t addr, uint32_t ep_type) {
    switch (ep_type) {
        case USBC_EP_TYPE_EP0:
            usb_device_ep0_read_data_complete(addr);
            break;

        case USBC_EP_TYPE_TX:
            return -1;

        case USBC_EP_TYPE_RX:
            usb_device_rx_read_data_complete(addr);
            break;

        default:
            return -1;
    }

    return 0;
}

static inline void usb_device_rx_send_stall(uint32_t addr) {
    usb_set_bit16(USBC_BP_RXCSR_D_SEND_STALL, USBC_REG_RXCSR(addr));
}

static inline uint32_t usb_device_rx_get_ep_stall(uint32_t addr) {
    return usb_get_bit16(USBC_BP_RXCSR_D_SENT_STALL, USBC_REG_RXCSR(addr));
}

static inline void usb_device_rx_clear_stall(uint32_t addr) {
    usb_clear_bit16(USBC_BP_RXCSR_D_SEND_STALL, USBC_REG_RXCSR(addr));
    usb_clear_bit16(USBC_BP_RXCSR_D_SENT_STALL, USBC_REG_RXCSR(addr));
}

static inline void usb_device_ep0_flush_fifo(uint32_t addr) {
    writew(1 << USBC_BP_CSR0_D_FLUSH_FIFO, USBC_REG_CSR0(addr));
}

static inline void usb_device_tx_flush_fifo(uint32_t addr) {
    writew((1 << USBC_BP_TXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_TXCSR_D_FLUSH_FIFO),
           USBC_REG_TXCSR(addr));
}

static inline void usb_device_rx_flush_fifo(uint32_t addr) {
    writew((1 << USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_RXCSR_D_FLUSH_FIFO),
           USBC_REG_RXCSR(addr));
}

/**
 * Set the default address for the USB device.
 *
 * @param husb The USB device handle.
 */
void usb_device_set_address_default(uint32_t husb);

/**
 * Set the address for the USB device.
 *
 * @param husb The USB device handle.
 * @param address The address to set.
 */
void usb_device_set_address(uint32_t husb, uint8_t address);

/**
 * Query the transfer mode of the USB device.
 *
 * @param husb The USB device handle.
 * @return The transfer mode.
 */
uint32_t usb_device_query_transfer_mode(uint32_t husb);

/**
 * Configure the transfer mode of the USB device.
 *
 * @param husb The USB device handle.
 * @param ts_type The transfer mode type.
 * @param speed_mode The speed mode.
 */
void usb_device_config_transfer_mode(uint32_t husb, uint8_t ts_type, uint8_t speed_mode);

/**
 * Switch the USB device connection on or off.
 *
 * @param husb The USB device handle.
 * @param is_on Whether to turn the connection on (1) or off (0).
 */
void usb_device_connect_switch(uint32_t husb, uint32_t is_on);

/**
 * Query the power status of the USB device.
 *
 * @param husb The USB device handle.
 * @return The power status.
 */
uint32_t usb_device_query_power_status(uint32_t husb);

/**
 * Configure an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ts_type The transfer mode type.
 * @param ep_type The endpoint type.
 * @param is_double_fifo Whether to use double FIFO or not.
 * @param ep_maxpkt The maximum packet size of the endpoint.
 * @return 0 on success, -1 on failure.
 */
int usb_device_config_ep(uint32_t husb, uint32_t ts_type, uint32_t ep_type, uint32_t is_double_fifo, uint32_t ep_maxpkt);

/**
 * Configure a default endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 0 on success, -1 on failure.
 */
int usb_device_config_ep_default(uint32_t husb, uint32_t ep_type);

/**
 * Configure an endpoint of the USB device to use DMA.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 0 on success, -1 on failure.
 */
int usb_device_config_ep_dma(uint32_t husb, uint32_t ep_type);

/**
 * Clear the DMA configuration for an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 0 on success, -1 on failure.
 */
int usb_device_clear_ep_dma(uint32_t husb, uint32_t ep_type);

/**
 * Get the stall status of an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 1 if stalled, 0 if not stalled.
 */
int usb_device_get_ep_stall(uint32_t husb, uint32_t ep_type);

/**
 * Send a stall condition on an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 0 on success, -1 on failure.
 */
int usb_device_ep_send_stall(uint32_t husb, uint32_t ep_type);

/**
 * Clear the stall condition on an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 0 on success, -1 on failure.
 */
int usb_device_ep_clear_stall(uint32_t husb, uint32_t ep_type);

/**
 * Get the setup end status of the control endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @return The setup end status.
 */
uint32_t usb_device_ctrl_get_setup_end(uint32_t husb);

/**
 * Clear the setup end status of the control endpoint of the USB device.
 *
 * @param husb The USB device handle.
 */
void usb_device_ctrl_clear_setup_end(uint32_t husb);

/**
 * Check the write data status of an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @param complete Whether the write operation is complete (1) or not (0).
 * @return 0 on success, -1 on failure.
 */
int usb_device_write_data_status(uint32_t husb, uint32_t ep_type, uint32_t complete);

/**
 * Check the read data status of an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @param complete Whether the read operation is complete (1) or not (0).
 * @return 0 on success, -1 on failure.
 */
int usb_device_read_data_status(uint32_t husb, uint32_t ep_type, uint32_t complete);

/**
 * Check if there is ready data to be read from an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 1 if ready, 0 if not ready.
 */
uint32_t usb_device_get_read_data_ready(uint32_t husb, uint32_t ep_type);

/**
 * Check if an endpoint of the USB device is ready to write data.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 1 if ready, 0 if not ready.
 */
uint32_t usb_device_get_write_data_ready(uint32_t husb, uint32_t ep_type);

/**
 * Check if the FIFO of an endpoint of the USB device is empty and ready to write data.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 * @return 1 if ready, 0 if not ready.
 */
uint32_t usb_device_get_write_data_ready_fifo_empty(uint32_t husb, uint32_t ep_type);

/**
 * Enable ISO update for the USB device.
 *
 * @param husb The USB device handle.
 * @return 0 on success, -1 on failure.
 */
int usb_device_iso_update_enable(uint32_t husb);

/**
 * Flush the FIFO of an endpoint of the USB device.
 *
 * @param husb The USB device handle.
 * @param ep_type The endpoint type.
 */
void usb_device_flush_fifo(uint32_t husb, uint32_t ep_type);


#endif// __USB_DEVICE_H__