/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_DMA_H__
#define __USB_DMA_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-usb.h"

/**
 * @brief Initialize USB DMA
 *
 * This function is used to initialize USB DMA.
 *
 * @param husb The USB controller handle
 * 
 * @return 0 on success
 */
int usb_dma_init(uint64_t husb);

/**
 * @brief Request a USB DMA channel
 *
 * This function is used to request a USB DMA channel.
 *
 * @return The DMA index on success, 0 if no available DMA channel is found
 */
int usb_dma_request(void);

/**
 * @brief Release a USB DMA channel
 *
 * This function is used to release a USB DMA channel.
 *
 * @param dma_index The DMA index to release
 * 
 * @return 0 on success, -1 on failure
 */
int usb_dma_release(uint32_t dma_index);

/**
 * @brief Set the parameters of a USB DMA channel
 *
 * This function is used to set the parameters of a USB DMA channel.
 *
 * @param dma_index The DMA index to configure
 * @param trans_dir The transfer direction (IN or OUT)
 * @param ep The endpoint number
 * 
 * @return 0 on success, -1 on failure
 */
int usb_dma_setting(uint32_t dma_index, uint32_t trans_dir, uint32_t ep);

/**
 * @brief Set the packet length of a USB DMA channel
 *
 * This function is used to set the packet length of a USB DMA channel.
 *
 * @param dma_index The DMA index to configure
 * @param pkt_len The packet length to set
 * 
 * @return 0 on success, -1 on failure
 */
int usb_dma_set_pktlen(uint32_t dma_index, uint32_t pkt_len);

/**
 * @brief Start a USB DMA transfer
 *
 * This function is used to start a USB DMA transfer.
 *
 * @param dma_index The DMA index to use
 * @param addr The address of the buffer for the transfer
 * @param bytes The number of bytes to transfer
 * 
 * @return 0 on success, -1 on failure
 */
int usb_dma_start(uint32_t dma_index, uint32_t addr, uint32_t bytes);

/**
 * @brief Stop a USB DMA transfer
 *
 * This function is used to stop a USB DMA transfer.
 *
 * @param dma_index The DMA index to stop
 * 
 * @return 0 on success, -1 on failure
 */
int usb_dma_stop(uint32_t dma_index);

/**
 * @brief Query the interrupt status of a USB DMA channel
 *
 * This function is used to query the interrupt status of a USB DMA channel.
 *
 * @param dma_index The DMA index to query
 * 
 * @return The interrupt status of the DMA channel
 */
int usb_dma_int_query(uint32_t dma_index);

/**
 * @brief Clear the interrupt status of all USB DMA channels
 *
 * This function is used to clear the interrupt status of all USB DMA channels.
 *
 * @return 0 on success, -1 on failure
 */
int usb_dma_int_clear(void);

#endif// __USB_DMA_H__