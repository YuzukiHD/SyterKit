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

#define SUNXI_USB_EP0_BUFFER_SIZE (512)
#define HIGH_SPEED_EP_MAX_PACKET_SIZE (512)
#define FULL_SPEED_EP_MAX_PACKET_SIZE (64)
#define BULK_FIFOSIZE (512)
#define RX_BUFF_SIZE (512)
#define SUNXI_USB_CTRL_EP_INDEX 0
#define SUNXI_USB_BULK_IN_EP_INDEX 1  /* tx */
#define SUNXI_USB_BULK_OUT_EP_INDEX 2 /* rx */

static sunxi_udc_t sunxi_udc_source;
static sunxi_ubuf_t sunxi_ubuf;
static sunxi_usb_setup_req_t *sunxi_udev_active;

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

void sunxi_usb_attach(uint32_t device_type) {
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

    sunxi_usb_dma_init(sunxi_udc_source.usbc_hd);

    usb_device_connect_switch(sunxi_udc_source.usbc_hd, USBC_DEVICE_SWITCH_OFF);

    /* Close clock */
    sunxi_usb_clk_deinit();

    /* request dma channal for usb send and recv */
    sunxi_udc_source.dma_send_channal = sunxi_usb_dma_request();
    if (!sunxi_udc_source.dma_send_channal) {
        printk(LOG_LEVEL_ERROR, "USB: unable to request dma for usb send data\n");
        goto sunxi_usb_init_fail;
    }
    printk(LOG_LEVEL_TRACE, "USB: dma send ch %d\n", sunxi_udc_source.dma_send_channal);
    sunxi_udc_source.dma_recv_channal = sunxi_usb_dma_request();
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
    sunxi_usb_dma_setting(sunxi_udc_source.dma_send_channal, USB_DMA_FROM_DRAM_TO_HOST, SUNXI_USB_BULK_IN_EP_INDEX);
    sunxi_usb_dma_set_pktlen(sunxi_udc_source.dma_send_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE);

    /* config dma for recv */
    sunxi_usb_dma_setting(sunxi_udc_source.dma_recv_channal, USB_DMA_FROM_HOST_TO_DRAM, SUNXI_USB_BULK_OUT_EP_INDEX);
    sunxi_usb_dma_set_pktlen(sunxi_udc_source.dma_recv_channal, HIGH_SPEED_EP_MAX_PACKET_SIZE);

    /* disable all interrupt */
    usb_controller_int_disable_usb_misc_all(sunxi_udc_source.usbc_hd);
    usb_controller_int_disable_ep_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_RX);
    usb_controller_int_disable_ep_all(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_TX);

    /* enable reset、resume、suspend  interrupt */
    usb_controller_int_enable_usb_misc_unit(sunxi_udc_source.usbc_hd, USBC_INTUSB_SUSPEND | USBC_INTUSB_RESUME | USBC_INTUSB_RESET | USBC_INTUSB_SOF);

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
        sunxi_usb_dma_release(sunxi_udc_source.dma_send_channal);
    }
    if (sunxi_udc_source.dma_recv_channal) {
        sunxi_usb_dma_release(sunxi_udc_source.dma_recv_channal);
    }
    if (sunxi_udc_source.usbc_hd) {
        usb_controller_close_otg(sunxi_udc_source.usbc_hd);
    }
    return -1;
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