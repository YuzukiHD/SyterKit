/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_H__
#define __USB_H__

#include "reg/reg-usb.h"

/*
 * Descriptor types
 */
#define USB_DT_DEVICE 0x01
#define USB_DT_CONFIG 0x02
#define USB_DT_STRING 0x03
#define USB_DT_INTERFACE 0x04
#define USB_DT_ENDPOINT 0x05
#define USB_DT_DEVICE_QUALIFIER 0x06

#define USB_DT_HID (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL (USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB (USB_TYPE_CLASS | 0x09)

/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE 18
#define USB_DT_CONFIG_SIZE 9
#define USB_DT_INTERFACE_SIZE 9
#define USB_DT_ENDPOINT_SIZE 7
#define USB_DT_ENDPOINT_AUDIO_SIZE 9 /* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE 7
#define USB_DT_HID_SIZE 9

enum usb_device_speed {
	USB_SPEED_LOW,
	USB_SPEED_FULL, /* usb 1.1 */
	USB_SPEED_HIGH, /* usb 2.0 */
	USB_SPEED_RESERVED
};

/*
 * standard usb descriptor structures
 */
struct usb_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType; /* 0x5 */
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} __attribute__((packed));

struct usb_interface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType; /* 0x04 */
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __attribute__((packed));

struct usb_configuration_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType; /* 0x2 */
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __attribute__((packed));

struct usb_device_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType; /* 0x01 */
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} __attribute__((packed));

struct usb_qualifier_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint8_t bNumConfigurations;
	uint8_t breserved;
} __attribute__((packed));

struct usb_string_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType; /* 0x03 */
	uint16_t wData[0];
} __attribute__((packed));

struct usb_generic_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
} __attribute__((packed));

struct usb_device_request {
	uint8_t request_type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} __attribute__((packed));

typedef struct sunxi_udc {
	uint64_t usbc_hd;
	uint32_t address;	  /* device address, allocated by the host */
	uint32_t speed;		  /* flag: is it high speed? */
	uint32_t bulk_ep_max; /* maximum packet size for bulk endpoints */
	uint32_t fifo_size;	  /* size of the FIFO */
	uint32_t bulk_in_addr;
	uint32_t bulk_out_addr;
	uint32_t dma_send_channal;
	uint32_t dma_recv_channal;
	struct usb_device_request standard_reg;
} sunxi_udc_t;

typedef struct sunxi_ubuf {
	uint8_t *rx_base_buffer;	/* base address for bulk transfer */
	uint8_t *rx_req_buffer;		/* buffer for the request phase of bulk transfer */
	uint32_t rx_req_length;		/* length of data in the request phase of bulk transfer */
	uint32_t rx_ready_for_data; /* flag indicating completion of data reception */
	uint32_t request_size;		/* size of the data to be sent */
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

#define sunxi_usb_module_init(name, state_init, state_exit, state_reset, standard_req_op, nonstandard_req_op, state_loop, dma_rx_isr, dma_tx_isr) \
	sunxi_usb_setup_req_t setup_req_##name = {state_init, state_exit, state_reset, standard_req_op, nonstandard_req_op, state_loop, dma_rx_isr, dma_tx_isr};

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

/**
 * @brief Run the USB device state machine loop once
 *
 * This function runs the USB device state machine loop once by calling
 * sunxi_udev_active->state_loop(&sunxi_ubuf) and returning the result.
 *
 * @param none
 * @return The result of sunxi_udev_active->state_loop(&sunxi_ubuf)
 */
int sunxi_usb_extern_loop();

/**
 * @brief Reset the bulk endpoints of the USB controller
 *
 * This function resets the bulk endpoints of the Allwinner A64 USB controller.
 * It configures the bulk-in and bulk-out endpoints for data transfer. The steps
 * involved are as follows:
 *
 * 1. Get the index of the currently active endpoint.
 * 2. Configure the bulk-in endpoint for transmitting data to the host.
 *    - Set the transfer type to bulk (USBC_TS_TYPE_BULK).
 *    - Set the endpoint type to transmit (USBC_EP_TYPE_TX).
 *    - Set the maximum packet size for the endpoint based on sunxi_udc_source.bulk_ep_max.
 *    - Configure the FIFO buffer with a size of sunxi_udc_source.fifo_size and the base address sunxi_udc_source.bulk_out_addr.
 *    - Enable interrupts for the endpoint.
 * 3. Configure the bulk-out endpoint for receiving data from the host.
 *    - Set the transfer type to bulk (USBC_TS_TYPE_BULK).
 *    - Set the endpoint type to receive (USBC_EP_TYPE_RX).
 *    - Set the maximum packet size for the endpoint based on sunxi_udc_source.bulk_ep_max.
 *    - Configure the FIFO buffer with a size of sunxi_udc_source.fifo_size and the base address sunxi_udc_source.bulk_in_addr.
 *    - Enable interrupts for the endpoint.
 * 4. Restore the previously active endpoint.
 *
 * @param none
 * @return none
 */
void sunxi_usb_bulk_ep_reset();

/**
 * @brief Start receiving data by DMA
 *
 * This function starts receiving data by DMA. It configures the USB controller
 * to use DMA for receiving, flushes the cache, enables DMA transfer, and
 * restores the active endpoint after the transfer is started.
 *
 * @param mem_base  The base address of the memory buffer
 * @param length    The length of the data to be received
 * @return 0 on success, -1 on failure
 */
int sunxi_usb_start_recv_by_dma(void *mem_base, uint32_t length);

/**
 * @brief Send a setup packet
 *
 * This function sends a setup packet over USB. If the length is zero, it sends
 * a zero-length packet. Otherwise, it selects the appropriate FIFO, writes
 * the packet to the FIFO, and calls sunxi_usb_write_complete() to handle the
 * completion of the write operation.
 *
 * @param length  The length of the setup packet
 * @param buffer  The buffer containing the setup packet
 */
void sunxi_usb_send_setup(uint32_t length, void *buffer);

/**
 * @brief Set USB device address
 *
 * This function sets the USB device address to the specified address. It updates
 * the address value in the sunxi_udc_source structure and calls usb_device_read_data_status()
 * to handle the status phase of the control transfer.
 *
 * @param address The USB device address to set
 * @return SUNXI_USB_REQ_SUCCESSED on success
 */
int sunxi_usb_set_address(uint32_t address);

/**
 * @brief Send data over USB
 *
 * This function sends data over USB. It sets the rx_ready_for_data flag to 0
 * in the sunxi_ubuf structure and calls sunxi_usb_write_fifo() to write the
 * data to the USB FIFO.
 *
 * @param buffer       The buffer containing the data to send
 * @param buffer_size  The size of the data buffer
 * @return 0 on success, -1 on failure
 */
int sunxi_usb_send_data(void *buffer, uint32_t buffer_size);

/**
 * @brief Get the maximum number of endpoints
 *
 * This function returns the maximum number of endpoints supported by the USB controller.
 *
 * @return The maximum number of endpoints
 */
int sunxi_usb_get_ep_max(void);

/**
 * @brief Get the IN endpoint type
 *
 * This function returns the type of the IN endpoint.
 *
 * @return The IN endpoint type
 */
int sunxi_usb_get_ep_in_type(void);

/**
 * @brief Get the OUT endpoint type
 *
 * This function returns the type of the OUT endpoint.
 *
 * @return The OUT endpoint type
 */
int sunxi_usb_get_ep_out_type(void);

#endif// __USB_H__