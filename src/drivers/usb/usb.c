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

#include "gic.h"
#include "usb.h"
#include "usb_controller.h"
#include "usb_defs.h"
#include "usb_device.h"

#define SUNXI_USB_EP0_BUFFER_SIZE (512)
#define HIGH_SPEED_EP_MAX_PACKET_SIZE (512)
#define FULL_SPEED_EP_MAX_PACKET_SIZE (64)
#define BULK_FIFOSIZE (512)
#define RX_BUFF_SIZE (512)
#define SUNXI_USB_CTRL_EP_INDEX 0
#define SUNXI_USB_BULK_IN_EP_INDEX 1  /* tx */
#define SUNXI_USB_BULK_OUT_EP_INDEX 2 /* rx */

static uint8_t sunxi_usb_ep0_buffer[SUNXI_USB_EP0_BUFFER_SIZE];

static sunxi_udc_t sunxi_udc_source;
static sunxi_ubuf_t sunxi_ubuf;
static sunxi_usb_setup_req_t *sunxi_udev_active;

static uint8_t usb_dma_trans_unaliged_bytes;
static uint8_t *usb_dma_trans_unaligned_buf;
static int dma_rec_flag;
static uint64_t dma_rec_addr;
static uint64_t dma_rec_size;

static void sunxi_usb_bulk_ep_reset() {
    uint8_t old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
    /* tx */
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_IN_EP_INDEX);
    usb_device_config_ep(sunxi_udc_source.usbc_hd, USBC_TS_TYPE_BULK, USBC_EP_TYPE_TX, 1, sunxi_udc_source.bulk_ep_max & 0x7ff);
    usb_controller_config_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, 1, sunxi_udc_source.fifo_size, (uint32_t) sunxi_udc_source.bulk_out_addr);
    usb_controller_int_enable_ep(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_BULK_IN_EP_INDEX);

    /* rx */
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);
    usb_device_config_ep(sunxi_udc_source.usbc_hd, USBC_TS_TYPE_BULK, USBC_EP_TYPE_RX, 1, sunxi_udc_source.bulk_ep_max & 0x7ff);
    usb_controller_config_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1, sunxi_udc_source.fifo_size, (uint32_t) sunxi_udc_source.bulk_in_addr);
    usb_controller_int_enable_ep(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, SUNXI_USB_BULK_OUT_EP_INDEX);

    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
    return;
}

void sunxi_usb_attach_module(uint32_t device_type) {
    sunxi_usb_module_reg(SUNXI_USB_DEVICE_DETECT);
    switch (device_type) {
        case SUNXI_USB_DEVICE_DETECT:
            sunxi_usb_module_reg(SUNXI_USB_DEVICE_DETECT);
            break;
        default:
            printk(LOG_LEVEL_ERROR, "USB: unknown device, type id = %d\n", device_type);
            break;
    }
}

int sunxi_usb_init() {
    uint32_t reg_val = 0;
    static uint8_t rx_base_buffer[RX_BUFF_SIZE];

    if (sunxi_udev_active->state_init()) {
        printk(LOG_LEVEL_ERROR, "USB: fail to init usb device\n");
        return -1;
    }

    printk(LOG_LEVEL_TRACE, "Init udc controller source\n");
    /* Init udc controller source */
    memset(&sunxi_udc_source, 0, sizeof(sunxi_udc_t));

    sunxi_udc_source.usbc_hd = usb_controller_open_otg(0);

    if (sunxi_udc_source.usbc_hd == 0) {
        printk(LOG_LEVEL_ERROR, "USB: usb_controller_open_otg failed\n");
        return -1;
    }

    usb_dma_init(sunxi_udc_source.usbc_hd);

    usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_OFF);

    /* Close clock */
    sunxi_usb_clk_deinit();

    /* request dma channal for usb send and recv */
    sunxi_udc_source.dma_send_channal = usb_dma_request();
    if (!sunxi_udc_source.dma_send_channal) {
        printk(LOG_LEVEL_ERROR, "USB: unable to request dma for usb send data\n");
        goto sunxi_usb_init_fail;
    }
    printk(LOG_LEVEL_TRACE, "USB: dma send ch %d\n", sunxi_udc_source.dma_send_channal);
    sunxi_udc_source.dma_recv_channal = usb_dma_request();
    if (!sunxi_udc_source.dma_recv_channal) {
        printk(LOG_LEVEL_ERROR, "USB: unable to request dma for usb receive data\n");
        goto sunxi_usb_init_fail;
    }
    printk(LOG_LEVEL_TRACE, "USB: dma recv ch %d\n", sunxi_udc_source.dma_recv_channal);

    /* init usb info */
    sunxi_udc_source.address = 0;
    sunxi_udc_source.speed = USB_SPEED_HIGH;
    sunxi_udc_source.bulk_ep_max = HIGH_SPEED_EP_MAX_PACKET_SIZE;
    sunxi_udc_source.fifo_size = BULK_FIFOSIZE;
    sunxi_udc_source.bulk_in_addr = 100;
    sunxi_udc_source.bulk_out_addr = sunxi_udc_source.bulk_in_addr + sunxi_udc_source.fifo_size * 2;

    /* init usb buffer */
    memset(&sunxi_ubuf, 0, sizeof(sunxi_ubuf_t));

    /* We use static memory for this moment */
    sunxi_ubuf.rx_base_buffer = rx_base_buffer;
    if (!sunxi_ubuf.rx_base_buffer) {
        printk(LOG_LEVEL_ERROR, "USB: %s:alloc memory fail\n");
        goto sunxi_usb_init_fail;
    }
    sunxi_ubuf.rx_req_buffer = sunxi_ubuf.rx_base_buffer;

    /* open usb clock */
    sunxi_usb_clk_init();

    /* disable OTG ID detect and set to device */
    usb_controller_force_id_status(sunxi_udc_source.usbc_hd, USBC_ID_TYPE_DEVICE);

    /* Force VBUS to HIGH */
    usb_controller_force_vbus_valid(sunxi_udc_source.usbc_hd, USBC_VBUS_TYPE_HIGH);

    /* disconnect usb */
    usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_OFF);

    /* set pull up for dp dm and id */
    usb_controller_dpdm_pull_enable(sunxi_udc_source.usbc_hd);
    usb_controller_id_pull_enable(sunxi_udc_source.usbc_hd);

    /* Set to use PIO mode */
    usb_controller_select_bus(sunxi_udc_source.usbc_hd, USBC_IO_TYPE_PIO, 0, 0);

    /* mapping SRAM buffer */
    usb_controller_config_fifo_base(sunxi_udc_source.usbc_hd, 0);

    /* set usb to HS mode and Bulk */
    usb_device_config_transfer_mode(sunxi_udc_source.usbc_hd, USBC_TS_TYPE_BULK, USBC_TS_MODE_HS);

    /* config dma for send */
    usb_dma_setting(sunxi_udc_source.dma_send_channal, USB_DMA_FROM_DRAM_TO_HOST, SUNXI_USB_BULK_IN_EP_INDEX);
    usb_dma_set_pktlen(sunxi_udc_source.dma_send_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE);

    /* config dma for recv */
    usb_dma_setting(sunxi_udc_source.dma_recv_channal, USB_DMA_FROM_HOST_TO_DRAM, SUNXI_USB_BULK_OUT_EP_INDEX);
    usb_dma_set_pktlen(sunxi_udc_source.dma_recv_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE);

    /* disable all interrupt */
    usb_controller_int_disable_usb_misc_all(sunxi_udc_source.usbc_hd);
    usb_controller_int_disable_ep_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
    usb_controller_int_disable_ep_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);

    /* enable reset、resume、suspend  interrupt */
    usb_controller_int_enable_usb_misc_uint(sunxi_udc_source.usbc_hd, USBC_INTUSB_SUSPEND | USBC_INTUSB_RESUME | USBC_INTUSB_RESET | USBC_INTUSB_SOF);

    /* enable ep interrupt */
    usb_controller_int_enable_ep(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);

    /* reset all ep */
    sunxi_usb_bulk_ep_reset();

    /* open usb device */
    usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_ON);

    /* set bit 1  ->  0 */
    reg_val = readl(SUNXI_USB0_BASE + USBC_REG_o_PHYCTL);
    reg_val &= ~(0x01 << 1);
    writel(reg_val, SUNXI_USB0_BASE + USBC_REG_o_PHYCTL);

    reg_val = readl(SUNXI_USB0_BASE + USBC_REG_o_PHYCTL);
    reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
    reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
    writel(reg_val, SUNXI_USB0_BASE + USBC_REG_o_PHYCTL);

    gic_enable(AW_IRQ_USB_OTG);

    sunxi_usb_dump(sunxi_udc_source.address, 0);

    return 0;

sunxi_usb_init_fail:
    if (sunxi_udc_source.dma_send_channal) {
        usb_dma_release(sunxi_udc_source.dma_send_channal);
    }
    if (sunxi_udc_source.dma_recv_channal) {
        usb_dma_release(sunxi_udc_source.dma_recv_channal);
    }
    if (sunxi_udc_source.usbc_hd) {
        usb_controller_close_otg(sunxi_udc_source.usbc_hd);
    }
    return -1;
}

static void sunxi_usb_clear_all_irq(void) {
    usb_controller_int_clear_ep_pending_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);
    usb_controller_int_clear_ep_pending_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
    usb_controller_int_clear_misc_pending_all(sunxi_udc_source.usbc_hd);
}

static void sunxi_usb_read_complete(uint32_t husb, uint32_t ep_type, uint32_t complete) {
    usb_device_read_data_status(husb, ep_type, complete);
    if (ep_type == USBC_EP_TYPE_EP0) {
        /* clear data end */
        if (complete) {
            usb_device_ep0_clear_setup_end(husb);
        }
        /* clear irq */
        usb_controller_int_clear_ep_pending(husb, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);
    }
    return;
}

static int sunxi_usb_read_ep0_data(void *buffer, uint32_t data_type) {
    uint32_t fifo_count = 0;
    uint32_t fifo = 0;
    int ret = 0;
    uint32_t old_ep_index = 0;

    old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
    fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_CTRL_EP_INDEX);
    fifo_count = usb_controller_read_len_from_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
    if (!data_type) {
        if (fifo_count != 8) {
            printk(LOG_LEVEL_ERROR, "USB: ep0 fifo_count %d is not 8\n", fifo_count);
            return -1;
        }
    }
    usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, fifo_count, (void *) buffer);
    sunxi_usb_read_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0, 1);
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
    return ret;
}

static int sunxi_usb_set_address(uint8_t address) {
    usb_device_set_address(sunxi_udc_source.usbc_hd, address);
    if (usb_device_query_transfer_mode(sunxi_udc_source.usbc_hd) == USBC_TS_MODE_HS) {
        sunxi_udc_source.speed = USB_SPEED_HIGH;
        sunxi_udc_source.fifo_size = HIGH_SPEED_EP_MAX_PACKET_SIZE;
        printk(LOG_LEVEL_TRACE, "USB: usb speed: HIGH\n");
    } else {
        sunxi_udc_source.speed = USB_SPEED_FULL;
        sunxi_udc_source.fifo_size = FULL_SPEED_EP_MAX_PACKET_SIZE;
        printk(LOG_LEVEL_TRACE, "USB: usb speed: FULL\n");
    }
    return SUNXI_USB_REQ_SUCCESSED;
}

static int ep0_recv_op(void) {
    uint32_t old_ep_index = 0;
    int ret = 0;
    static uint32_t ep0_stage;

    if (!ep0_stage) {
        memset(&sunxi_udc_source.standard_reg, 0, sizeof(struct usb_device_request));
    }

    old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_CTRL_EP_INDEX);

    /* clear stall status */
    if (usb_device_get_ep_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0)) {
        printk(LOG_LEVEL_ERROR, "USB: handle_ep0: ep0 stall\n");
        usb_device_ep_clear_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
        ret = -1;
        goto ep0_recv_op_err;
    }

    /* clear setup end */
    if (usb_device_ctrl_get_setup_end(sunxi_udc_source.usbc_hd)) {
        usb_device_ctrl_clear_setup_end(sunxi_udc_source.usbc_hd);
    }

    /*检查读ep0数据是否完成*/
    if (usb_device_get_read_data_ready(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0)) {
        uint32_t status;

        if (!ep0_stage) {
            status = sunxi_usb_read_ep0_data(&sunxi_udc_source.standard_reg, ep0_stage);
        } else {
            status = sunxi_usb_read_ep0_data(sunxi_usb_ep0_buffer, ep0_stage);
        }
        if (status != 0) {
            printk(LOG_LEVEL_ERROR, "USB: read_request failed\n");
            ret = -1;
            goto ep0_recv_op_err;
        }
    } else {
        /*此情况通常由于ep0发送空包引起，可以不处理*/
        printk(LOG_LEVEL_TRACE, "USB: ep0 rx data is not ready\n");
        if (sunxi_udc_source.address) {
            sunxi_usb_set_address(sunxi_udc_source.address & 0xff);
            printk(LOG_LEVEL_ERROR, "USB: set address 0x%x ok\n", sunxi_udc_source.address);
            sunxi_udc_source.address = 0;
        }
        goto ep0_recv_op_err;
    }
    /* Check data */
    if (USB_REQ_TYPE_STANDARD == (sunxi_udc_source.standard_reg.request_type & USB_REQ_TYPE_MASK)) {
        ret = SUNXI_USB_REQ_UNMATCHED_COMMAND;
        /* standard */
        switch (sunxi_udc_source.standard_reg.request) {
            case USB_REQ_GET_STATUS: /*   0x00*/
            {
                /* device-to-host */
                if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_STATUS, &sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
                }
                break;
            }
            case USB_REQ_CLEAR_FEATURE: /*   0x01*/
            {
                /* host-to-device */
                if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    ret = sunxi_udev_active->standard_req_op(USB_REQ_CLEAR_FEATURE, &sunxi_udc_source.standard_reg, NULL);
                }
                break;
            }
            case USB_REQ_SET_FEATURE: /*   0x03*/
            {
                /* host-to-device */
                if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    ret = sunxi_udev_active->standard_req_op(USB_REQ_SET_FEATURE, &sunxi_udc_source.standard_reg, NULL);
                }

                break;
            }
            case USB_REQ_SET_ADDRESS: /*   0x05*/
            {
                /* host-to-device */
                if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_DEVICE ==
                        (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        /* receiver is device */
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_SET_ADDRESS, &sunxi_udc_source.standard_reg, NULL);
                    }
                }

                break;
            }
            case USB_REQ_GET_DESCRIPTOR: /*   0x06*/
            {
                /* device-to-host */
                if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_DEVICE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_DESCRIPTOR, &sunxi_udc_source.standard_reg, NULL);
                    }
                }

                break;
            }
            case USB_REQ_SET_DESCRIPTOR: /*   0x07*/
            {
                /* host-to-device */
                if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_DEVICE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        /*there is some problem*/
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_SET_DESCRIPTOR, &sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
                    }
                }

                break;
            }
            case USB_REQ_GET_CONFIGURATION: /*   0x08*/
            {
                /* device-to-host */
                if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_DEVICE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_CONFIGURATION, &sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
                    }
                }

                break;
            }
            case USB_REQ_SET_CONFIGURATION: /*   0x09*/
            {
                /* host-to-device */
                if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_DEVICE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_SET_CONFIGURATION, &sunxi_udc_source.standard_reg, NULL);
                    }
                }

                break;
            }
            case USB_REQ_GET_INTERFACE: /*   0x0a*/
            {
                /* device-to-host */
                if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_DEVICE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_GET_INTERFACE, &sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer);
                    }
                }

                break;
            }
            case USB_REQ_SET_INTERFACE: /*   0x0b*/
            {
                /* host-to-device */
                if (USB_DIR_OUT == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_INTERFACE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        ret = sunxi_udev_active->standard_req_op(USB_REQ_SET_INTERFACE, &sunxi_udc_source.standard_reg, NULL);
                    }
                }

                break;
            }
            case USB_REQ_SYNCH_FRAME: /*   0x0b*/
            {
                /* device-to-host */
                if (USB_DIR_IN == (sunxi_udc_source.standard_reg.request_type & USB_REQ_DIRECTION_MASK)) {
                    if (USB_RECIP_INTERFACE == (sunxi_udc_source.standard_reg.request_type & USB_REQ_RECIPIENT_MASK)) {
                        /*there is some problem*/
                        if (!ep0_stage) {
                            ep0_stage = 1;
                        } else {
                            ret = sunxi_udev_active->standard_req_op(USB_REQ_SYNCH_FRAME, &sunxi_udc_source.standard_reg, NULL);
                            ep0_stage = 0;
                        }
                    }
                }

                break;
            }
            default: {
                printk(LOG_LEVEL_ERROR, "USB: sunxi usb err: unknown usb out request to device\n");
                usb_device_ep_send_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
                ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
                ep0_stage = 0;
                break;
            }
        }
    } else {
        /* Non-Standard Req */
        printk(LOG_LEVEL_ERROR, "USB: non standard req\n");
        ret = sunxi_udev_active->nonstandard_req_op(USB_REQ_GET_STATUS, &sunxi_udc_source.standard_reg, sunxi_usb_ep0_buffer, ep0_stage);
        if (ret == SUNXI_USB_REQ_DATA_HUNGRY) {
            ep0_stage = 1;
        } else if (ret == SUNXI_USB_REQ_SUCCESSED) {
            ep0_stage = 0;
        } else if (ret < 0) {
            ep0_stage = 0;
            printk(LOG_LEVEL_ERROR, "USB: unkown request_type(%d)\n", sunxi_udc_source.standard_reg.request_type);
            usb_device_ep_send_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
        }
    }

ep0_recv_op_err:
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
    return ret;
}

void sunxi_usb_ep_reset(void) {
    sunxi_usb_bulk_ep_reset();
}

static int eptx_send_op() {
    // TODO: ep send operating
    return 0;
}

/*
 * Function name: sunxi_usb_recv_by_dma_isr
 *
 * @param p_arg: Pointer to the argument passed to the ISR. (Input)
 *
 * Description:
 * This function is the interrupt service routine for USB receive by DMA.
 * It takes a pointer to the argument passed to the ISR as an input parameter.
 * The function selects the RX endpoint, clears the DMA flag, and transfers the data using IO mode.
 * If there are unaligned bytes in the DMA transfer, it reads the packet manually and returns the status.
 * If the current DMA transfer is not a complete packet, it manually clears the interrupt.
 * Lastly, it selects the previous active endpoint and calls the RX DMA ISR of the active USB device.
 */
static void sunxi_usb_recv_by_dma_isr(void *p_arg) {
    uint32_t old_ep_idx = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX); /* Select RXEP */

    /* Select IO mode for the data transfer */
    printk(LOG_LEVEL_TRACE, "USB: select io mode to transfer data\n");
    usb_device_clear_ep_dma(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);

    if (usb_dma_trans_unaliged_bytes) {
        uint32_t fifo, this_len;
        this_len = usb_controller_read_len_from_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
        fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);
        usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, this_len, usb_dma_trans_unaligned_buf);
        sunxi_usb_read_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1); /* Return status */
        usb_dma_trans_unaliged_bytes = 0;
    }

    /* If the current DMA transfer is not a complete packet, manually clear the interrupt */
    if (sunxi_ubuf.request_size % sunxi_udc_source.bulk_ep_max) {
        usb_device_read_data_status(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1);
    }

    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
    sunxi_udev_active->dma_rx_isr(p_arg);
}

/*
 * Function name: sunxi_usb_send_by_dma_isr
 *
 * Description:
 * This function is the interrupt service routine for sending data using DMA.
 * It calls the DMA transmit interrupt service routine specified by 'dma_tx_isr' of the active USB device.
 *
 * Parameters:
 * - p_arg: A void pointer that can be used to pass arguments to the DMA transmit interrupt service routine.
 */
static void sunxi_usb_send_by_dma_isr(void *p_arg) {
    sunxi_udev_active->dma_tx_isr(p_arg);
}

/*
 * Function name: eprx_recv_op
 *
 * Description:
 * This function performs the USB receive operation on the RX endpoint. It selects the RX endpoint, checks if it is stalled,
 * and clears the stall if necessary. If the data is ready to be read, it reads the length from the FIFO and checks if DMA is in progress.
 * If DMA is in progress, it reads the packet manually and returns the status.
 * Otherwise, if the buffer is not ready to receive data, it reads the data into the buffer and sets the buffer flag.
 * If the buffer is already set, it does nothing and leaves it to DMA.
 * Lastly, it selects the previous active endpoint and returns 0.
 *
 * Return:
 * 0 - Success
 */
static int eprx_recv_op(void) {
    uint32_t old_ep_index;
    uint32_t this_len;
    uint32_t fifo;

    old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);

    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);

    if (usb_device_get_ep_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX)) {
        usb_device_ep_clear_stall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
        printk(LOG_LEVEL_ERROR, "sunxi ubs read error: usb rx ep is busy already\n");
    } else {
        if (usb_device_get_read_data_ready(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX)) {
            this_len = usb_controller_read_len_from_fifo(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
            if (dma_rec_flag == 1) {
                uint64_t len_left = this_len > dma_rec_size ? this_len - dma_rec_size : 0;
                fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);
                usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, dma_rec_size, (void *) dma_rec_addr);
                if (len_left) {
                    sunxi_ubuf.rx_req_length = usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, this_len, sunxi_ubuf.rx_req_buffer);
                }
                printk(LOG_LEVEL_TRACE, "USB: fake rx dma\n");
                sunxi_usb_recv_by_dma_isr(NULL);

                dma_rec_flag = 0;
            } else if (!sunxi_ubuf.rx_ready_for_data) {
                fifo = usb_controller_select_fifo(sunxi_udc_source.usbc_hd, SUNXI_USB_BULK_OUT_EP_INDEX);

                memset(sunxi_ubuf.rx_req_buffer, 0, 64);
                sunxi_ubuf.rx_req_length = usb_controller_read_packet(sunxi_udc_source.usbc_hd, fifo, this_len, sunxi_ubuf.rx_req_buffer);
                sunxi_ubuf.rx_ready_for_data = 1;

                printk(LOG_LEVEL_TRACE, "USB: read ep bytes 0x%x\n", sunxi_ubuf.rx_req_length);
                sunxi_usb_read_complete(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, 1); /*返回状态*/
            } else {
                printk(LOG_LEVEL_TRACE, "USB: eprx do nothing and left it to dma\n");
            }
        } else {
            printk(LOG_LEVEL_TRACE, "USB: sunxi usb rxdata not ready\n");
        }
    }
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_index);
    return 0;
}

void sunxi_usb_irq() {
    uint8_t misc_irq = 0;
    uint16_t tx_irq = 0;
    uint16_t rx_irq = 0;
    uint32_t dma_irq = 0;
    uint32_t old_ep_idx = 0;

    /* Save index */
    uint8_t old_ep_index = usb_controller_get_active_ep(sunxi_udc_source.usbc_hd);

    /* Read status registers */
    misc_irq = usb_controller_int_misc_pending(sunxi_udc_source.usbc_hd);
    tx_irq = usb_controller_int_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);
    rx_irq = usb_controller_int_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
    dma_irq = usb_dma_int_query();

    /* RESET */
    if (misc_irq & USBC_INTUSB_RESET) {
        printk(LOG_LEVEL_TRACE, "USB: IRQ: reset\n");
        usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_RESET);

        sunxi_usb_clear_all_irq();

        usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, 0);
        usb_device_set_address_default(sunxi_udc_source.usbc_hd);

        sunxi_udc_source.address = 0;            /*default value*/
        sunxi_udc_source.speed = USB_SPEED_HIGH; /*default value*/

        usb_dma_stop(sunxi_udc_source.dma_recv_channal);
        usb_dma_stop(sunxi_udc_source.dma_send_channal);

        usb_dma_set_pktlen(sunxi_udc_source.dma_send_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE);
        usb_dma_set_pktlen(sunxi_udc_source.dma_recv_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE);

        sunxi_ubuf.rx_ready_for_data = 0;
        sunxi_udev_active->state_reset();

        return;
    }

    /* RESUME ont handle, just clear interrupt */
    if (misc_irq & USBC_INTUSB_RESUME) {
        printk(LOG_LEVEL_TRACE, "USB: IRQ: resume\n");
        /* clear interrupt */
        usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_RESUME);
    }

    /* SUSPEND */
    if (misc_irq & USBC_INTUSB_SUSPEND) {
        printk(LOG_LEVEL_TRACE, "USB: IRQ: suspend\n");
        /* clear interrupt */
        usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_SUSPEND);
    }

    /* DISCONNECT */
    if (misc_irq & USBC_INTUSB_DISCONNECT) {
        printk(LOG_LEVEL_TRACE, "USB: IRQ: disconnect\n");
        usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_DISCONNECT);
        return;
    }

    /* SOF */
    if (misc_irq & USBC_INTUSB_SOF) {
        printk(LOG_LEVEL_TRACE, "USB: IRQ: SOF\n");
        usb_controller_int_disable_usb_misc_uint(sunxi_udc_source.usbc_hd, USBC_INTUSB_SOF);
        usb_controller_int_clear_misc_pending(sunxi_udc_source.usbc_hd, USBC_INTUSB_SOF);
    }

    /* ep0 */
    if (tx_irq & (1 << SUNXI_USB_CTRL_EP_INDEX)) {
        printk(LOG_LEVEL_TRACE, "USB: IRQ: EP0\n");
        usb_controller_int_clear_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_CTRL_EP_INDEX);
        /* handle ep0 ops */
        ep0_recv_op();
    }

    /* tx endpoint data transfers */
    if (tx_irq & (1 << SUNXI_USB_BULK_IN_EP_INDEX)) {
        printk(LOG_LEVEL_TRACE, "USB: tx irq occur\n");
        /* Clear the interrupt bit by setting it to 1 */
        usb_controller_int_clear_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX, SUNXI_USB_BULK_IN_EP_INDEX);
        eptx_send_op();
    }

    /* rx endpoint data transfers */
    if (rx_irq & (1 << SUNXI_USB_BULK_OUT_EP_INDEX)) {
        printk(LOG_LEVEL_TRACE, "USB: rx irq occur\n");
        /* Clear the interrupt bit by setting it to 1 */
        usb_controller_int_clear_ep_pending(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX, SUNXI_USB_BULK_OUT_EP_INDEX);
        eprx_recv_op();
    }

    if (dma_irq & (1 << SUNXI_USB_BULK_IN_EP_INDEX)) {
        printk(LOG_LEVEL_TRACE, "USB: tx dma\n");
        sunxi_usb_send_by_dma_isr(NULL);
    }

    if (dma_irq & (1 << SUNXI_USB_BULK_OUT_EP_INDEX)) {
        printk(LOG_LEVEL_TRACE, "USB: rx dma\n");
        sunxi_usb_recv_by_dma_isr(NULL);
    }

    usb_dma_int_clear();
    usb_controller_select_active_ep(sunxi_udc_source.usbc_hd, old_ep_idx);
    return;
}

void sunxi_usb_attach() {
    int i = 0;
    while (1) {
        if (gic_is_pending(AW_IRQ_USB_OTG)) {
            sunxi_usb_irq();
        }

        i = 10;
        while (i--) {
            sunxi_udev_active->state_loop(&sunxi_ubuf);
        }
    }
}

void sunxi_usb_dump(uint32_t usbc_base, uint32_t ep_index) {
    uint32_t old_ep_index = 0;

    if (ep_index >= 0) {
        old_ep_index = readw(usbc_base + USBC_REG_o_EPIND);
        writew(ep_index, (usbc_base + USBC_REG_o_EPIND));
        printk(LOG_LEVEL_TRACE, "old_ep_index = %d, ep_index = %d\n", old_ep_index, ep_index);
    }

    printk(LOG_LEVEL_TRACE, "USBC_REG_o_FADDR         = 0x%08x\n", readb(usbc_base + USBC_REG_o_FADDR));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_PCTL          = 0x%08x\n", readb(usbc_base + USBC_REG_o_PCTL));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_INTTx         = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTTx));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_INTRx         = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTRx));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_INTTxE        = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTTxE));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_INTRxE        = 0x%08x\n", readw(usbc_base + USBC_REG_o_INTRxE));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_INTUSB        = 0x%08x\n", readb(usbc_base + USBC_REG_o_INTUSB));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_INTUSBE       = 0x%08x\n", readb(usbc_base + USBC_REG_o_INTUSBE));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_EPIND         = 0x%08x\n", readw(usbc_base + USBC_REG_o_EPIND));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_TXMAXP        = 0x%08x\n", readw(usbc_base + USBC_REG_o_TXMAXP));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_CSR0          = 0x%08x\n", readw(usbc_base + USBC_REG_o_CSR0));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_TXCSR         = 0x%08x\n", readw(usbc_base + USBC_REG_o_TXCSR));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXMAXP        = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXMAXP));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXCSR         = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXCSR));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_COUNT0        = 0x%08x\n", readw(usbc_base + USBC_REG_o_COUNT0));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXCOUNT       = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXCOUNT));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_TXTYPE        = 0x%08x\n", readb(usbc_base + USBC_REG_o_TXTYPE));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_NAKLIMIT0     = 0x%08x\n", readb(usbc_base + USBC_REG_o_NAKLIMIT0));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_TXINTERVAL    = 0x%08x\n", readb(usbc_base + USBC_REG_o_TXINTERVAL));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXTYPE        = 0x%08x\n", readb(usbc_base + USBC_REG_o_RXTYPE));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXINTERVAL    = 0x%08x\n", readb(usbc_base + USBC_REG_o_RXINTERVAL));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_CONFIGDATA    = 0x%08x\n", readb(usbc_base + USBC_REG_o_CONFIGDATA));

    printk(LOG_LEVEL_TRACE, "USBC_REG_o_DEVCTL        = 0x%08x\n", readb(usbc_base + USBC_REG_o_DEVCTL));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_TXFIFOSZ      = 0x%08x\n", readb(usbc_base + USBC_REG_o_TXFIFOSZ));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXFIFOSZ      = 0x%08x\n", readb(usbc_base + USBC_REG_o_RXFIFOSZ));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_TXFIFOAD      = 0x%08x\n", readw(usbc_base + USBC_REG_o_TXFIFOAD));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_RXFIFOAD      = 0x%08x\n", readw(usbc_base + USBC_REG_o_RXFIFOAD));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_VEND0         = 0x%08x\n", readb(usbc_base + USBC_REG_o_VEND0));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_VEND1         = 0x%08x\n", readb(usbc_base + USBC_REG_o_VEND1));

    printk(LOG_LEVEL_TRACE, "=====================================\n");
    printk(LOG_LEVEL_TRACE, "TXFADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_TXFADDRx));
    printk(LOG_LEVEL_TRACE, "TXHADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_TXHADDRx));
    printk(LOG_LEVEL_TRACE, "TXHPORTx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_TXHPORTx));
    printk(LOG_LEVEL_TRACE, "RXFADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_RXFADDRx));
    printk(LOG_LEVEL_TRACE, "RXHADDRx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_RXHADDRx));
    printk(LOG_LEVEL_TRACE, "RXHPORTx(%d)              = 0x%08x\n", ep_index, readb(usbc_base + USBC_REG_o_RXHPORTx));
    printk(LOG_LEVEL_TRACE, "RPCOUNTx(%d)              = 0x%08x\n", ep_index, (uint32_t) readw(usbc_base + USBC_REG_o_RPCOUNT));
    printk(LOG_LEVEL_TRACE, "=====================================\n");

    printk(LOG_LEVEL_TRACE, "USBC_REG_o_ISCR          = 0x%08x\n", (uint32_t) readl(usbc_base + USBC_REG_o_ISCR));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_PHYCTL        = 0x%08x\n", (uint32_t) readl(usbc_base + USBC_REG_o_PHYCTL));
    printk(LOG_LEVEL_TRACE, "USBC_REG_o_PHYBIST       = 0x%08x\n", (uint32_t) readl(usbc_base + USBC_REG_o_PHYBIST));

    if (ep_index >= 0) {
        writew(old_ep_index, (usbc_base + USBC_REG_o_EPIND));
    }

    return;
}