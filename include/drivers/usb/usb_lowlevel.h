/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_LOWLEVEL_H__
#define __DRIVERS_USB_LOWLEVEL_H__

#include <drivers/usb/usb_types.h>

int usb_dma_init(uintptr_t husb);
int usb_dma_request(void);
int usb_dma_release(uint32_t dma_index);
int usb_dma_setting(uint32_t dma_index, uint32_t trans_dir, uint32_t ep);
int usb_dma_set_pktlen(uint32_t dma_index, uint32_t pkt_len);
int usb_dma_start(uint32_t dma_index, uint32_t addr, uint32_t bytes);
int usb_dma_stop(uint32_t dma_index);
int usb_dma_int_query(void);
int usb_dma_int_clear(void);

#endif /* __DRIVERS_USB_LOWLEVEL_H__ */
