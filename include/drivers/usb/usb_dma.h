/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_DMA_H__
#define __USB_DMA_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-usb.h"

int sunxi_usb_dma_init(uint32_t husb);

int sunxi_usb_dma_request(void);

int sunxi_usb_dma_release(uint32_t dma_index);

int sunxi_usb_dma_setting(uint32_t dma_index, uint32_t trans_dir, uint32_t ep);

int sunxi_usb_dma_set_pktlen(uint32_t dma_index, uint32_t pkt_len);

#endif// __USB_DMA_H__