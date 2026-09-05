/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_TYPES_H__
#define __DRIVERS_USB_TYPES_H__

/** @file
 * @brief Common USB protocol types, constants, and controller metadata.
 */

#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <log.h>
#include <malloc.h>

#define SUNXI_USB_COMPATIBLE "allwinner,sunxi-usb"
#define SUNXI_USB_MAX_CONTROLLERS 3U

/** @brief Compatibility aliases used by legacy USB functions. */
#define printk_trace pr_trace
#define printk_debug pr_debug
#define printk_info pr_info
#define printk_warning pr_warn
#define printk_error pr_err
#define smalloc malloc
#define sfree free

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/** @brief USB device and interface class codes. */
#define USB_CLASS_PER_INTERFACE 0
#define USB_CLASS_AUDIO 1
#define USB_CLASS_COMM 2
#define USB_CLASS_HID 3
#define USB_CLASS_PHYSICAL 5
#define USB_CLASS_PRINTER 7
#define USB_CLASS_MASS_STORAGE 8
#define USB_CLASS_HUB 9
#define USB_CLASS_DATA 10
#define USB_CLASS_APP_SPEC 0xfe
#define USB_CLASS_VENDOR_SPEC 0xff

/** @brief Bit-field values used by bmRequestType. */
#define USB_TYPE_STANDARD (0x00 << 5)
#define USB_TYPE_CLASS (0x01 << 5)
#define USB_TYPE_VENDOR (0x02 << 5)
#define USB_TYPE_RESERVED (0x03 << 5)

#define USB_RECIP_DEVICE 0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT 0x02
#define USB_RECIP_OTHER 0x03

#define USB_DIR_OUT 0x00
#define USB_DIR_IN 0x80

/** @brief USB descriptor types and fixed descriptor sizes. */
#define USB_DT_DEVICE 0x01
#define USB_DT_CONFIG 0x02
#define USB_DT_STRING 0x03
#define USB_DT_INTERFACE 0x04
#define USB_DT_ENDPOINT 0x05
#define USB_DT_DEVICE_QUALIFIER 0x06
#define USB_DT_OTHER_SPEED_CONFIG 0x07

#if defined(CONFIG_USBD_HS)
#define USB_DT_QUAL USB_DT_DEVICE_QUALIFIER
#endif

#define USB_DT_HID (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL (USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB (USB_TYPE_CLASS | 0x09)

#define USB_DT_DEVICE_SIZE 18
#define USB_DT_CONFIG_SIZE 9
#define USB_DT_INTERFACE_SIZE 9
#define USB_DT_ENDPOINT_SIZE 7
#define USB_DT_ENDPOINT_AUDIO_SIZE 9
#define USB_DT_HUB_NONVAR_SIZE 7
#define USB_DT_HID_SIZE 9

#define USB_ENDPOINT_NUMBER_MASK 0x0f
#define USB_ENDPOINT_DIR_MASK 0x80
#define USB_ENDPOINT_XFERTYPE_MASK 0x03
#define USB_ENDPOINT_XFER_CONTROL 0
#define USB_ENDPOINT_XFER_ISOC 1
#define USB_ENDPOINT_XFER_BULK 2
#define USB_ENDPOINT_XFER_INT 3

/** @brief USB packet identifiers. */
#define USB_PID_UNDEF_0 0xf0
#define USB_PID_OUT 0xe1
#define USB_PID_ACK 0xd2
#define USB_PID_DATA0 0xc3
#define USB_PID_PING 0xb4
#define USB_PID_SOF 0xa5
#define USB_PID_NYET 0x96
#define USB_PID_DATA2 0x87
#define USB_PID_SPLIT 0x78
#define USB_PID_IN 0x69
#define USB_PID_NAK 0x5a
#define USB_PID_DATA1 0x4b
#define USB_PID_PREAMBLE 0x3c
#define USB_PID_ERR 0x3c
#define USB_PID_SETUP 0x2d
#define USB_PID_STALL 0x1e
#define USB_PID_MDATA 0x0f

/** @brief Standard and HID request codes. */
#define USB_REQ_GET_STATUS 0x00
#define USB_REQ_CLEAR_FEATURE 0x01
#define USB_REQ_SET_FEATURE 0x03
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_DESCRIPTOR 0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE 0x0a
#define USB_REQ_SET_INTERFACE 0x0b
#define USB_REQ_SYNCH_FRAME 0x0c

#define USB_REQ_GET_REPORT 0x01
#define USB_REQ_GET_IDLE 0x02
#define USB_REQ_GET_PROTOCOL 0x03
#define USB_REQ_SET_REPORT 0x09
#define USB_REQ_SET_IDLE 0x0a
#define USB_REQ_SET_PROTOCOL 0x0b

#if defined(CONFIG_USBD_HS)
#define USB_BCD_VERSION 0x0200
#else
#define USB_BCD_VERSION 0x0110
#endif

#define USB_REQ_DIRECTION_MASK 0x80
#define USB_REQ_TYPE_MASK 0x60
#define USB_REQ_RECIPIENT_MASK 0x1f
#define USB_REQ_DEVICE2HOST USB_DIR_IN
#define USB_REQ_HOST2DEVICE USB_DIR_OUT
#define USB_REQ_TYPE_STANDARD USB_TYPE_STANDARD
#define USB_REQ_TYPE_CLASS USB_TYPE_CLASS
#define USB_REQ_TYPE_VENDOR USB_TYPE_VENDOR
#define USB_REQ_RECIPIENT_DEVICE USB_RECIP_DEVICE
#define USB_REQ_RECIPIENT_INTERFACE USB_RECIP_INTERFACE
#define USB_REQ_RECIPIENT_ENDPOINT USB_RECIP_ENDPOINT
#define USB_REQ_RECIPIENT_OTHER USB_RECIP_OTHER

#define USB_STATUS_SELFPOWERED 0x01
#define USB_STATUS_REMOTEWAKEUP 0x02
#define USB_STATUS_HALT 0x01

#define USB_DESCRIPTOR_TYPE_DEVICE USB_DT_DEVICE
#define USB_DESCRIPTOR_TYPE_CONFIGURATION USB_DT_CONFIG
#define USB_DESCRIPTOR_TYPE_STRING USB_DT_STRING
#define USB_DESCRIPTOR_TYPE_INTERFACE USB_DT_INTERFACE
#define USB_DESCRIPTOR_TYPE_ENDPOINT USB_DT_ENDPOINT
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER USB_DT_DEVICE_QUALIFIER
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION USB_DT_OTHER_SPEED_CONFIG
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER 0x08
#define USB_DESCRIPTOR_TYPE_HID 0x21
#define USB_DESCRIPTOR_TYPE_REPORT 0x22

#define USB_ENDPOINT_HALT 0x00
#define USB_DEVICE_REMOTE_WAKEUP 0x01
#define USB_TEST_MODE 0x02

/** @brief USB bus speed negotiated by the device controller. */
enum usb_device_speed {
	USB_SPEED_LOW,
	USB_SPEED_FULL,
	USB_SPEED_HIGH,
	USB_SPEED_RESERVED,
};

/** @brief USB endpoint descriptor transferred on the control pipe. */
struct usb_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} __attribute__((packed));

/** @brief USB interface descriptor transferred on the control pipe. */
struct usb_interface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __attribute__((packed));

/** @brief USB configuration descriptor transferred on the control pipe. */
struct usb_configuration_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __attribute__((packed));

/** @brief USB device descriptor returned during enumeration. */
struct usb_device_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
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

/** @brief USB high-speed device qualifier descriptor. */
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

/** @brief Variable-length USB string descriptor header. */
struct usb_string_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wData[0];
} __attribute__((packed));

/** @brief Common header shared by class-specific descriptors. */
struct usb_generic_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
} __attribute__((packed));

/** @brief USB setup packet received on endpoint zero. */
struct usb_device_request {
	uint8_t request_type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} __attribute__((packed));

/** @brief Devicetree-provided controller and platform configuration. */
typedef struct sunxi_usb {
	int dt_node;
	uint8_t id;
	uint8_t phy_clock_gate_offset;
	uint8_t phy_reset_offset;
	uint8_t clock_gate_offset;
	uint8_t reset_offset;
	uint32_t irq;
	uintptr_t base;
	uintptr_t phy_clock_reg_base;
	uintptr_t clock_gate_reg_base;
	bool detected;
} sunxi_usb_t;

/** @brief MUSB register bit helpers shared by controller and gadget layers. */
#define usb_get_bit8(bp, reg) (read8(reg) & (1 << (bp)))
#define usb_get_bit16(bp, reg) (read16(reg) & (1 << (bp)))
#define usb_get_bit32(bp, reg) (read32(reg) & (1 << (bp)))
#define usb_set_bit8(bp, reg) (write8((reg), (read8(reg) | (1 << (bp)))))
#define usb_set_bit16(bp, reg) (write16((reg), (read16(reg) | (1 << (bp)))))
#define usb_set_bit32(bp, reg) (write32((reg), (read32(reg) | (1 << (bp)))))
#define usb_clear_bit8(bp, reg) (write8((reg), (read8(reg) & ~(1 << (bp)))))
#define usb_clear_bit16(bp, reg) (write16((reg), (read16(reg) & ~(1 << (bp)))))
#define usb_clear_bit32(bp, reg) (write32((reg), (read32(reg) & ~(1 << (bp)))))

#endif /* __DRIVERS_USB_TYPES_H__ */
