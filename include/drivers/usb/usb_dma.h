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



#endif// __USB_DMA_H__