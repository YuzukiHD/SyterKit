/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_H__
#define __USB_H__

#include "reg/reg-usb.h"

enum usb_device_speed {
    USB_SPEED_LOW,
    USB_SPEED_FULL, /* usb 1.1 */
    USB_SPEED_HIGH, /* usb 2.0 */
    USB_SPEED_RESERVED
};

struct usb_device_request {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed));

typedef struct sunxi_udc {
    uint64_t usbc_hd;
    uint32_t address;     /* device address, allocated by the host */
    uint32_t speed;       /* flag: is it high speed? */
    uint32_t bulk_ep_max; /* maximum packet size for bulk endpoints */
    uint32_t fifo_size;   /* size of the FIFO */
    uint32_t bulk_in_addr;
    uint32_t bulk_out_addr;
    uint32_t dma_send_channal;
    uint32_t dma_recv_channal;
    struct usb_device_request standard_reg;
} sunxi_udc_t;

typedef struct sunxi_ubuf {
    uint8_t *rx_base_buffer;    /* base address for bulk transfer */
    uint8_t *rx_req_buffer;     /* buffer for the request phase of bulk transfer */
    uint32_t rx_req_length;     /* length of data in the request phase of bulk transfer */
    uint32_t rx_ready_for_data; /* flag indicating completion of data reception */
    uint32_t request_size;      /* size of the data to be sent */
} sunxi_ubuf_t;

typedef struct sunxi_usb_setup_req_s {
    int (*state_init)(void);
    int (*state_exit)(void);
    void (*state_reset)(void);
    int (*standard_req_op)(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer);
    int (*nonstandard_req_op)(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status);
    int (*state_loop)(void *sunxi_udc);
    void (*dma_rx_isr)(void *p_arg);
    void (*dma_tx_isr)(void *p_arg);
} sunxi_usb_setup_req_t;

#define SUNXI_USB_DEVICE_DETECT (1)
#define SUNXI_USB_DEVICE_MASS (2)

#define sunxi_usb_module_init(name, state_init, state_exit, state_reset, \
                              standard_req_op, nonstandard_req_op,       \
                              state_loop, dma_rx_isr, dma_tx_isr)        \
    sunxi_usb_setup_req_t setup_req_##name = {                           \
            state_init, state_exit, state_reset, standard_req_op,        \
            nonstandard_req_op, state_loop, dma_rx_isr, dma_tx_isr};

#define sunxi_usb_module_reg(name) sunxi_udev_active = &setup_req_##name

#define sunxi_usb_module_ext(name) extern sunxi_usb_setup_req_t setup_req_##name

/* export usb module */
sunxi_usb_module_ext(SUNXI_USB_DEVICE_DETECT);
sunxi_usb_module_ext(SUNXI_USB_DEVICE_MASS);

/* USB IO Wrzpper */
#define usb_get_bit8(bp, reg) (read8(reg) & (1 << (bp)))
#define usb_get_bit16(bp, reg) (read16(reg) & (1 << (bp)))
#define usb_get_bit32(bp, reg) (read32(reg) & (1 << (bp)))

#define usb_set_bit8(bp, reg) (write8((reg), (read8(reg) | (1 << (bp)))))
#define usb_set_bit16(bp, reg) (write16((reg), (read16(reg) | (1 << (bp)))))
#define usb_set_bit32(bp, reg) (write32((reg), (read32(reg) | (1 << (bp)))))

#define usb_clear_bit8(bp, reg) (write8((reg), (read8(reg) & (~(1 << (bp))))))
#define usb_clear_bit16(bp, reg) (write16((reg), (read16(reg) & (~(1 << (bp))))))
#define usb_clear_bit32(bp, reg) (write32((reg), (read32(reg) & (~(1 << (bp))))))

/* Error Codes */
#define SUNXI_USB_REQ_SUCCESSED (0)
#define SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED (-1)
#define SUNXI_USB_REQ_UNKNOWN_COMMAND (-2)
#define SUNXI_USB_REQ_UNMATCHED_COMMAND (-3)
#define SUNXI_USB_REQ_DATA_HUNGRY (-4)
#define SUNXI_USB_REQ_OP_ERR (-5)

/**
 * @brief Attach a module to the USB controller
 *
 * This function attaches a module to the USB controller. It registers the
 * specified USB module and switches to the specified device type. Currently,
 * two types of devices are supported: USB device detect and USB mass storage.
 * If an unknown device type is specified, an error message is printed.
 *
 * @param device_type The type of the device to attach
 *                    - SUNXI_USB_DEVICE_DETECT: USB device detect module
 *                    - SUNXI_USB_DEVICE_MASS: USB mass storage module
 *
 * @return none
 */
void sunxi_usb_attach_module(uint32_t device_type);

/**
 * @brief Initialize the USB controller
 *
 * This function initializes the USB controller. It first checks if
 * it can successfully initialize the USB device, then initializes the UDC controller
 * source and requests DMA channels for data sending and receiving. Next, it configures
 * the USB device by setting the transfer mode and speed, configuring DMA, etc.
 * Finally, it enables interrupts and opens the USB device.
 *
 * @return 0 on success, -1 on failure
 *
 * @param none
 */
int sunxi_usb_init();

/**
 * @brief Dump the USB controller registers for a specific endpoint
 *
 * This function dumps the USB controller registers for a specific endpoint of the
 * Allwinner A64 USB controller.
 *
 * @param usbc_base The base address of the USB controller
 * @param ep_index  The index of the endpoint to dump registers for
 * @return none
 */
void sunxi_usb_dump(uint32_t usbc_base, uint32_t ep_index);

/**
 * @brief Reset all endpoints of the USB controller
 *
 * This function resets all endpoints of the Allwinner A64 USB controller by calling
 * the sunxi_usb_bulk_ep_reset() function to reset the bulk endpoints.
 *
 * @param none
 * @return none
 */
void sunxi_usb_ep_reset();

/**
 * @brief Handle the USB interrupt.
 *
 * This function is responsible for handling various USB interrupts, including RESET, RESUME, SUSPEND,
 * DISCONNECT, SOF, endpoint 0 (EP0), and data transfers for both transmit (TX) and receive (RX) endpoints.
 * It also handles DMA interrupts for both TX and RX endpoints.
 *
 * @param None
 * @return None
 */
void sunxi_usb_irq();

/**
 * @brief Attach the USB device to the USB controller
 *
 * This function attaches the USB device to the USB controller and enters a loop
 * to handle USB events. It continuously checks for pending USB interrupts,
 * handles the USB interrupt by calling sunxi_usb_irq(), and then processes
 * the USB device state by calling sunxi_udev_active->state_loop(&sunxi_ubuf)
 * multiple times.
 *
 * @param none
 * @return none
 */
void sunxi_usb_attach();

#endif// __USB_H__