/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include "usb.h"
#include "usb_defs.h"

const uint8_t normal_lang_id[8] = {0x04, 0x03, 0x09, 0x04, '\0'};
const uint8_t sunxi_usb_mass_serial_num0[32] = "20101201120001";
const uint8_t sunxi_usb_mass_manufacturer[32] = "AllWinner Technology";
const uint8_t sunxi_usb_mass_product[32] = "USB Mass Storage";

#define SUNXI_USB_STRING_LANGIDS (0)
#define SUNXI_USB_STRING_IMANUFACTURER (1)
#define SUNXI_USB_STRING_IPRODUCT (2)
#define SUNXI_USB_STRING_ISERIALNUMBER (3)

#define SUNXI_USB_MASS_DEV_MAX (4)

/* clang-format off */
uint8_t *sunxi_usb_mass_dev[SUNXI_USB_MASS_DEV_MAX] = {
        normal_lang_id,
        sunxi_usb_mass_serial_num0,
        sunxi_usb_mass_manufacturer,
        sunxi_usb_mass_product
};

const uint8_t inquiry_data[40] = {
        0x00, 0x80, 0x02, 0x02, 0x1f,
        0x00, 0x00, 0x00,
        'U', 'S', 'B', '2', '.', '0', 0x00, 0x00,
        'U', 'S', 'B', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e',
        0x00, 0x00, 0x00, 0x00, 0x00,
        '0', '1', '0', '0', '\0'
};

const uint8_t request_sense[20] = {
        0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
        0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00
};
/* clang-format on */

#define SUNXI_USB_MASS_IDLE (0)
#define SUNXI_USB_MASS_SETUP (1)
#define SUNXI_USB_MASS_SEND_DATA (2)
#define SUNXI_USB_MASS_RECEIVE_DATA (3)
#define SUNXI_USB_MASS_STATUS (4)

typedef struct mass_trans_set {
    uint8_t *base_recv_buffer;
    uint32_t act_recv_buffer;
    uint32_t recv_size;
    uint32_t to_be_recved_size;
    uint8_t *base_send_buffer;
    uint32_t act_send_buffer;
    uint32_t send_size;
    uint32_t flash_start;
    uint32_t flash_sectors;
} mass_trans_set_t;

#define SUNXI_MASS_RECV_MEM_SIZE (512 * 1024)
#define SUNXI_MASS_SEND_MEM_SIZE (512 * 1024)

static int sunxi_usb_mass_write_enable = 0;
static int sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;
static mass_trans_set_t trans_data;

static int usb_mass_usb_set_interface(struct usb_device_request *req) {
    sunxi_usb_dbg("set interface\n");
    /* Only support interface 0, alternate 0 */
    if ((0 == req->index) && (0 == req->value)) {
        sunxi_usb_ep_reset();
    } else {
        printk(LOG_LEVEL_ERROR, "USB MASS: invalid index and value, (0, %d), (0, %d)\n", req->index, req->value);
        return SUNXI_USB_REQ_OP_ERR;
    }
    return SUNXI_USB_REQ_SUCCESSED;
}

static int usb_mass_usb_set_address(struct usb_device_request *req) {
    uint8_t address = req->value & 0x7f;
    printk(LOG_LEVEL_TRACE, "set address 0x%x\n", address);
    return SUNXI_USB_REQ_SUCCESSED;
}