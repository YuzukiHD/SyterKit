/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_LOWLEVEL_H__
#define __DRIVERS_USB_LOWLEVEL_H__

#include <drivers/usb/usb_types.h>

/**
 * @brief Initialize DMA channel bookkeeping for a USB controller.
 *
 * @param husb The handle to the USB controller.
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_init(uintptr_t husb);

/**
 * @brief Allocate a free USB DMA channel.
 *
 * @return The allocated channel number, or zero when no channel is available.
 */
int usb_dma_request(void);

/**
 * @brief Release an allocated USB DMA channel.
 *
 * @param dma_index The DMA channel number.
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_release(uint32_t dma_index);

/**
 * @brief Configure the direction and endpoint for a DMA channel.
 *
 * @param dma_index The DMA channel number.
 * @param trans_dir The transfer direction.
 * @param ep The endpoint number or type.
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_setting(uint32_t dma_index, uint32_t trans_dir, uint32_t ep);

/**
 * @brief Set the packet length for a USB DMA channel.
 *
 * @param dma_index The DMA channel number.
 * @param pkt_len The packet length in bytes.
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_set_pktlen(uint32_t dma_index, uint32_t pkt_len);

/**
 * @brief Start a USB DMA transfer.
 *
 * @param dma_index The DMA channel number.
 * @param addr The 32-bit transfer buffer address.
 * @param bytes The transfer length in bytes.
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_start(uint32_t dma_index, uint32_t addr, uint32_t bytes);

/**
 * @brief Stop a USB DMA transfer.
 *
 * @param dma_index The DMA channel number.
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_stop(uint32_t dma_index);

/**
 * @brief Read the pending USB DMA interrupt status.
 *
 * @return The DMA status register value, or a negative error code on failure.
 */
int usb_dma_int_query(void);

/**
 * @brief Clear pending USB DMA interrupt status.
 *
 * @return Zero on success, or a negative error code on failure.
 */
int usb_dma_int_clear(void);

#endif /* __DRIVERS_USB_LOWLEVEL_H__ */
