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

/**
 * @brief Set the parameters for a USB DMA channel
 *
 * This function is used to configure the parameters for a specific USB DMA channel.
 *
 * @param husb The USB controller handle
 * @param dma_chan The DMA channel number
 * @param trans_dir The transfer direction (0: OUT, 1: IN)
 * @param ep_type The endpoint type
 * 
 * @return 0 on success, -1 on failure
 */
static int lowlevel_usb_dma_set_channal_para(uint64_t husb, uint32_t dma_chan, uint32_t trans_dir, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb; /**< The USB controller handle */
	uint32_t reg_val = 0;											/**< The value to be written to the register */
	if (usbc_otg == NULL) {
		return -1;
	}
	reg_val = readl(usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan); /**< Read the current value of the register */
	reg_val &= ~((1 << 4) | (0xf << 0));											/**< Clear the bits related to transfer direction and endpoint type */
	reg_val |= ((trans_dir & 1) << 4);												/**< Set the transfer direction bit in the register */
	reg_val |= ((ep_type & 0xf) << 0);												/**< Set the endpoint type bits in the register */
	writel(reg_val, usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan); /**< Write the updated value back to the register */
	return 0;
}

/**
 * @brief Set the packet length for a USB DMA channel
 *
 * This function is used to set the packet length for a specific USB DMA channel.
 *
 * @param husb The USB controller handle
 * @param dma_chan The DMA channel number
 * @param pkt_len The packet length to be set
 * 
 * @return 0 on success, -1 on failure
 */
static int lowlevel_usb_dma_set_packet_len(uint64_t husb, uint32_t dma_chan, uint32_t pkt_len) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb; /**< The USB controller handle */
	uint32_t reg_val = 0;											/**< The value to be written to the register */

	if (usbc_otg == NULL) {
		return -1;
	}

	reg_val = readl(usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan); /**< Read the current value of the register */
	reg_val &= ~(0x7ff << 16);														/**< Clear the bits related to packet length */

	/* 1650 burst len: datawidth is 32bit = 4byte, so burst len = pkt_len/4 */
	/* reg_val |=  (((pkt_len/4) & 0x7ff) << 16); */

	/* 1667 burst len: datawidth is 8bit, so burst len = 1byte */
	reg_val |= (((pkt_len) &0x7ff) << 16);											/**< Set the burst length bits in the register */
	writel(reg_val, usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan); /**< Write the updated value back to the register */

	return 0;
}

/**
 * @brief Start a USB DMA transfer on a specific channel
 *
 * This function is used to start a USB DMA transfer on a specific channel with the given address and packet length.
 *
 * @param husb The USB controller handle
 * @param dma_chan The DMA channel number
 * @param addr The address of the buffer to be transferred
 * @param pkt_len The length of the packet to be transferred
 * 
 * @return 0 on success, -1 on failure
 */
static int lowlevel_usb_dma_start(uint64_t husb, uint32_t dma_chan, uint32_t addr, uint32_t pkt_len) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb; /**< The USB controller handle */
	uint32_t reg_val;

	if (usbc_otg == NULL) {
		return -1;
	}

	if (pkt_len & 0x03) { /**< Check if the packet length is not a multiple of 4 bytes */
		return -1;
	}

	reg_val = readl(usbc_otg->base_addr + USBC_REG_o_DMA_ENABLE); /**< Read the current value of the DMA enable register */
	reg_val |= (1 << (dma_chan & 0xff));						  /**< Enable the specified DMA channel */
	writel(reg_val, usbc_otg->base_addr + USBC_REG_o_DMA_ENABLE); /**< Write the updated value back to the DMA enable register */

	writel(addr, usbc_otg->base_addr + USBC_REG_o_DMA_ADDR + 0x10 * dma_chan); /**< Set the address for the DMA transfer */

	/* 1650 datawidth is 32 bit. so size = len/4 */
	/* writel(pkt_len/4, usbc_otg->base_addr + USBC_REG_o_DMA_SIZE + 0x10 * dma_chan); */

	/* 1667 datawidth is 8bit */
	writel(pkt_len, usbc_otg->base_addr + USBC_REG_o_DMA_SIZE + 0x10 * dma_chan); /**< Set the size of the DMA transfer */

	reg_val = readl(usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan); /**< Read the current value of the DMA config register */
	reg_val |= (1U << 31);															/**< Set the DMA start bit */
	writel(reg_val, usbc_otg->base_addr + USBC_REG_o_DMA_CONFIG + 0x10 * dma_chan); /**< Write the updated value back to the DMA config register */

	return 0;
}

/**
 * @brief Stop a USB DMA transfer on a specific channel
 *
 * This function is used to stop a USB DMA transfer on a specific channel.
 *
 * @param husb The USB controller handle
 * @param dma_chan The DMA channel number
 * 
 * @return 0 on success, -1 on failure
 */
static int lowlevel_usb_dma_int_stop(uint64_t husb, uint32_t dma_chan) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb; /**< The USB controller handle */
	uint32_t reg_val;

	if (usbc_otg == NULL) {
		return -1;
	}

	reg_val = readl(usbc_otg->base_addr + USBC_REG_o_DMA_ENABLE); /**< Read the current value of the DMA enable register */
	reg_val &= ~(1 << (dma_chan & 0xff));						  /**< Disable the specified DMA channel */
	writel(reg_val, usbc_otg->base_addr + USBC_REG_o_DMA_ENABLE); /**< Write the updated value back to the DMA enable register */

	return 0;
}

/**
 * @brief Query the status of USB DMA transfers
 *
 * This function is used to query the status of USB DMA transfers.
 *
 * @param husb The USB controller handle
 * 
 * @return The status of DMA transfers on success, -1 on failure
 */
static int lowlevel_usb_dma_int_query(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb; /**< The USB controller handle */

	if (usbc_otg == NULL) {
		return -1;
	}

	return readl(usbc_otg->base_addr + USBC_REG_o_DMA_STATUS); /**< Read and return the current DMA status */
}

/**
 * @brief Clear the status of USB DMA transfers
 *
 * This function is used to clear the status of USB DMA transfers.
 *
 * @param husb The USB controller handle
 * 
 * @return 0 on success, -1 on failure
 */
static int lowlevel_usb_dma_int_clear(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb; /**< The USB controller handle */

	if (usbc_otg == NULL) {
		return -1;
	}

	writel(0xff, usbc_otg->base_addr + USBC_REG_o_DMA_STATUS); /**< Clear the DMA status register */

	return 0;
}

/**
 * @brief Check if the USB DMA index is valid
 *
 * This function is used to check if the USB DMA index is within range and is currently in use.
 *
 * @param dma_index The USB DMA index to check
 * 
 * @return 0 on success, -1 on failure
 */
static int usb_index_check(uint32_t dma_index) {
	if (dma_index > SUNXI_USB_DMA_MAX) { /**< Check if the DMA index is within range */
		printk_error("USB: dma %d is overrange\n", dma_index);
		return -1;
	}
	if (!usb_dma_used[dma_index]) { /**< Check if the DMA index is currently in use */
		printk_error("USB: dma %d is not used\n", dma_index);
		return -1;
	}
	return 0;
}

int usb_dma_init(uint64_t husb) {
	usb_hd = husb; /**< Set the USB handle */

	return 0;
}

int usb_dma_request(void) {
	for (int i = 1; i < SUNXI_USB_DMA_MAX; i++) {
		if (usb_dma_used[i] == 0) { /**< Check if the DMA channel is available */
			usb_dma_used[i] = 1;	/**< Mark the DMA channel as used */
			return i;
		}
	}

	return 0;
}

int usb_dma_release(uint32_t dma_index) {
	int ret = usb_index_check(dma_index);
	if (ret) {
		return ret;
	}

	usb_dma_used[dma_index] = 0; /**< Mark the DMA channel as unused */

	return 0;
}

int usb_dma_setting(uint32_t dma_index, uint32_t trans_dir, uint32_t ep) {
	int ret = usb_index_check(dma_index);
	if (ret) {
		return ret;
	}

	return lowlevel_usb_dma_set_channal_para(usb_hd, dma_index, trans_dir, ep);
}

int usb_dma_set_pktlen(uint32_t dma_index, uint32_t pkt_len) {
	int ret = usb_index_check(dma_index);
	if (ret) {
		return ret;
	}

	return lowlevel_usb_dma_set_packet_len(usb_hd, dma_index, pkt_len);
}

int usb_dma_start(uint32_t dma_index, uint32_t addr, uint32_t bytes) {
	int ret = usb_index_check(dma_index);
	if (ret) {
		return ret;
	}

	return lowlevel_usb_dma_start(usb_hd, dma_index, addr, bytes);
}

int usb_dma_stop(uint32_t dma_index) {
	int ret = usb_index_check(dma_index);
	if (ret) {
		return ret;
	}

	return lowlevel_usb_dma_int_stop(usb_hd, dma_index);
}

int usb_dma_int_query(uint32_t dma_index) {
	return lowlevel_usb_dma_int_query(usb_hd);
}

int usb_dma_int_clear(void) {
	return lowlevel_usb_dma_int_clear(usb_hd);
}
