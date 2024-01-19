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

/* 记录当前USB port所有的硬件信息 */
typedef struct usb_controller_otg {
    uint32_t port_num;
    uint64_t base_addr; /* usb base address 		*/

    uint32_t used; /* 是否正在被使用   		*/
    uint32_t no;   /* 在管理数组中的位置 		*/
} usb_controller_otg_t;

uint64_t usb_controller_open_otg(uint32_t otg_no);

int usb_controller_dma_set_channal_para(uint64_t husb, uint32_t dma_chan, uint32_t trans_dir, uint32_t ep_type);



#endif// __USB_CONTROLLER_H__