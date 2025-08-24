/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <sys-sdcard.h>

#include "usb.h"
#include "usb_defs.h"

const uint8_t normal_lang_id[8] = {0x04, 0x03, 0x09, 0x04, '\0'};
const uint8_t sunxi_usb_mass_serial_num0[32] = "20240127003501";
const uint8_t sunxi_usb_mass_manufacturer[32] = "Yuzuki HD";
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

#define CBWCDBLENGTH 16

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

struct umass_bbb_cbw_t {
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCDBLength;
	uint8_t CBWCDB[CBWCDBLENGTH];
} __attribute__((packed));

/* Command Status Wrapper */
struct umass_bbb_csw_t {
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} __attribute__((packed));

#define SUNXI_MASS_RECV_MEM_SIZE (512 * 1024)
#define SUNXI_MASS_SEND_MEM_SIZE (512 * 1024)

static int sunxi_usb_mass_write_enable = 0;
static int sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;
static mass_trans_set_t trans_data;

#define SCSI_CHANGE_DEF 0x40 /* Change Definition (Optional) */
#define SCSI_COMPARE 0x39	 /* Compare (O) */
#define SCSI_COPY 0x18		 /* Copy (O) */
#define SCSI_COP_VERIFY 0x3A /* Copy and Verify (O) */
#define SCSI_INQUIRY 0x12	 /* Inquiry (MANDATORY) */
#define SCSI_LOG_SELECT 0x4C /* Log Select (O) */
#define SCSI_LOG_SENSE 0x4D	 /* Log Sense (O) */
#define SCSI_MODE_SEL6 0x15	 /* Mode Select 6-byte (Device Specific) */
#define SCSI_MODE_SEL10 0x55 /* Mode Select 10-byte (Device Specific) */
#define SCSI_MODE_SEN6 0x1A	 /* Mode Sense 6-byte (Device Specific) */
#define SCSI_MODE_SEN10 0x5A /* Mode Sense 10-byte (Device Specific) */
#define SCSI_READ_BUFF 0x3C	 /* Read Buffer (O) */
#define SCSI_REQ_SENSE 0x03	 /* Request Sense (MANDATORY) */
#define SCSI_SEND_DIAG 0x1D	 /* Send Diagnostic (O) */
#define SCSI_TST_U_RDY 0x00	 /* Test Unit Ready (MANDATORY) */
#define SCSI_WRITE_BUFF 0x3B /* Write Buffer (O) */
/***************************************************************************
 * Commands Unique to Direct Access Devices %%%
 ***************************************************************************/
#define SCSI_COMPARE 0x39	   /* Compare (O) */
#define SCSI_FORMAT 0x04	   /* Format Unit (MANDATORY) */
#define SCSI_LCK_UN_CAC 0x36   /* Lock Unlock Cache (O) */
#define SCSI_PREFETCH 0x34	   /* Prefetch (O) */
#define SCSI_MED_REMOVL 0x1E   /* Prevent/Allow medium Removal (O) */
#define SCSI_READ6 0x08		   /* Read 6-byte (MANDATORY) */
#define SCSI_READ10 0x28	   /* Read 10-byte (MANDATORY) */
#define SCSI_RD_CAPAC 0x25	   /* Read Capacity (MANDATORY) */
#define SCSI_RD_DEFECT 0x37	   /* Read Defect Data (O) */
#define SCSI_READ_LONG 0x3E	   /* Read Long (O) */
#define SCSI_REASS_BLK 0x07	   /* Reassign Blocks (O) */
#define SCSI_RCV_DIAG 0x1C	   /* Receive Diagnostic Results (O) */
#define SCSI_RELEASE 0x17	   /* Release Unit (MANDATORY) */
#define SCSI_REZERO 0x01	   /* Rezero Unit (O) */
#define SCSI_SRCH_DAT_E 0x31   /* Search Data Equal (O) */
#define SCSI_SRCH_DAT_H 0x30   /* Search Data High (O) */
#define SCSI_SRCH_DAT_L 0x32   /* Search Data Low (O) */
#define SCSI_SEEK6 0x0B		   /* Seek 6-Byte (O) */
#define SCSI_SEEK10 0x2B	   /* Seek 10-Byte (O) */
#define SCSI_SEND_DIAG 0x1D	   /* Send Diagnostics (MANDATORY) */
#define SCSI_SET_LIMIT 0x33	   /* Set Limits (O) */
#define SCSI_START_STP 0x1B	   /* Start/Stop Unit (O) */
#define SCSI_SYNC_CACHE 0x35   /* Synchronize Cache (O) */
#define SCSI_VERIFY 0x2F	   /* Verify (O) */
#define SCSI_WRITE6 0x0A	   /* Write 6-Byte (MANDATORY) */
#define SCSI_WRITE10 0x2A	   /* Write 10-Byte (MANDATORY) */
#define SCSI_WRT_VERIFY 0x2E   /* Write and Verify (O) */
#define SCSI_WRITE_LONG 0x3F   /* Write Long (O) */
#define SCSI_WRITE_SAME 0x41   /* Write Same (O) */
#define SCSI_RD_FMT_CAPAC 0x23 /* Read format caacity */

/**
 * @brief USB MASS: Set interface
 *
 * This function handles the set interface request for the USB Mass Storage device.
 * It checks if the requested interface and alternate setting are valid. If they are,
 * it calls sunxi_usb_ep_reset() to reset the USB endpoints. Otherwise, it prints an
 * error message and returns SUNXI_USB_REQ_OP_ERR.
 *
 * @param req The USB device request
 * @return SUNXI_USB_REQ_SUCCESSED on success, SUNXI_USB_REQ_OP_ERR on error
 */
static int usb_mass_usb_set_interface(struct usb_device_request *req) {
	printk_trace("USB MASS: set interface\n");
	/* Only support interface 0, alternate 0 */
	if ((0 == req->index) && (0 == req->value)) {
		sunxi_usb_ep_reset();
	} else {
		printk_error("USB MASS: invalid index and value, (0, %d), (0, %d)\n", req->index, req->value);
		return SUNXI_USB_REQ_OP_ERR;
	}
	return SUNXI_USB_REQ_SUCCESSED;
}

/**
 * @brief USB MASS: Set address
 *
 * This function handles the set address request for the USB Mass Storage device.
 * It extracts the new address from the request and calls sunxi_usb_set_address()
 * to set the new address.
 *
 * @param req The USB device request
 * @return SUNXI_USB_REQ_SUCCESSED on success
 */
static int usb_mass_usb_set_address(struct usb_device_request *req) {
	uint8_t address = req->value & 0x7f;
	sunxi_usb_set_address(address);
	return SUNXI_USB_REQ_SUCCESSED;
}

/**
 * @brief USB MASS: Set configuration
 *
 * This function handles the set configuration request for the USB Mass Storage device.
 * It checks if the requested configuration is valid. If it is, it calls sunxi_usb_ep_reset()
 * to reset the USB endpoints. Otherwise, it prints an error message and returns SUNXI_USB_REQ_OP_ERR.
 *
 * @param req The USB device request
 * @return SUNXI_USB_REQ_SUCCESSED on success, SUNXI_USB_REQ_OP_ERR on error
 */
static int usb_mass_usb_set_configuration(struct usb_device_request *req) {
	printk_trace("set configuration\n");
	/* Only support 1 configuration so nak anything else */
	if (1 == req->value) {
		sunxi_usb_ep_reset();
	} else {
		printk_error("err: invalid wValue, (0, %d)\n", req->value);
		return SUNXI_USB_REQ_OP_ERR;
	}
	return SUNXI_USB_REQ_SUCCESSED;
}

/**
 * usb_mass_usb_get_descriptor - Get USB descriptors
 *
 * This function retrieves USB descriptors based on the provided request.
 *
 * @param req: Pointer to the USB device request structure
 * @param buffer: Pointer to the buffer to hold the descriptor data
 *
 * @return: Error code indicating the result of the function
 */
static int usb_mass_usb_get_descriptor(struct usb_device_request *req, uint8_t *buffer) {
	int ret = SUNXI_USB_REQ_SUCCESSED;

	// Get descriptor
	switch (req->value >> 8) {
		case USB_DT_DEVICE: {
			struct usb_device_descriptor *dev_dscrptr;
			printk_trace("USB MASS: get device descriptor\n");
			dev_dscrptr = (struct usb_device_descriptor *) buffer;
			memset((void *) dev_dscrptr, 0, sizeof(struct usb_device_descriptor));
			dev_dscrptr->bLength = min(req->length, sizeof(struct usb_device_descriptor));
			dev_dscrptr->bDescriptorType = USB_DT_DEVICE;
			dev_dscrptr->bcdUSB = 0x200;
			dev_dscrptr->bDeviceClass = 0;// Device class: Mass Storage
			dev_dscrptr->bDeviceSubClass = 0;
			dev_dscrptr->bDeviceProtocol = 0;
			dev_dscrptr->bMaxPacketSize0 = 0x40;
			dev_dscrptr->idVendor = 0x7d4a;
			dev_dscrptr->idProduct = 0x2b82;
			dev_dscrptr->bcdDevice = 0x200;
			dev_dscrptr->iManufacturer = SUNXI_USB_STRING_IMANUFACTURER;
			dev_dscrptr->iProduct = SUNXI_USB_STRING_IPRODUCT;
			dev_dscrptr->iSerialNumber = SUNXI_USB_STRING_ISERIALNUMBER;
			dev_dscrptr->bNumConfigurations = 1;

			sunxi_usb_send_setup(dev_dscrptr->bLength, buffer);
		} break;
		case USB_DT_CONFIG: {
			struct usb_configuration_descriptor *config_dscrptr;
			struct usb_interface_descriptor *inter_dscrptr;
			struct usb_endpoint_descriptor *ep_in, *ep_out;
			uint8_t bytes_remaining = req->length;
			uint8_t bytes_total = 0;

			printk_trace("USB MASS: get config descriptor\n");

			bytes_total = sizeof(struct usb_configuration_descriptor) + sizeof(struct usb_interface_descriptor) + sizeof(struct usb_endpoint_descriptor) +
						  sizeof(struct usb_endpoint_descriptor);

			memset(buffer, 0, bytes_total);

			config_dscrptr = (struct usb_configuration_descriptor *) (buffer + 0);
			inter_dscrptr = (struct usb_interface_descriptor *) (buffer + sizeof(struct usb_configuration_descriptor));
			ep_in = (struct usb_endpoint_descriptor *) (buffer + sizeof(struct usb_configuration_descriptor) + sizeof(struct usb_interface_descriptor));
			ep_out = (struct usb_endpoint_descriptor *) (buffer + sizeof(struct usb_configuration_descriptor) + sizeof(struct usb_interface_descriptor) +
														 sizeof(struct usb_endpoint_descriptor));

			/* Configuration */
			config_dscrptr->bLength = min(bytes_remaining, sizeof(struct usb_configuration_descriptor));
			config_dscrptr->bDescriptorType = USB_DT_CONFIG;
			config_dscrptr->wTotalLength = bytes_total;
			config_dscrptr->bNumInterfaces = 1;
			config_dscrptr->bConfigurationValue = 1;
			config_dscrptr->iConfiguration = 0;
			config_dscrptr->bmAttributes = 0xc0;
			config_dscrptr->bMaxPower = 0xFA;// Maximum current of 500ms (0xfa * 2)
			bytes_remaining -= config_dscrptr->bLength;
			/* interface */
			inter_dscrptr->bLength = min(bytes_remaining, sizeof(struct usb_interface_descriptor));
			inter_dscrptr->bDescriptorType = USB_DT_INTERFACE;
			inter_dscrptr->bInterfaceNumber = 0x00;
			inter_dscrptr->bAlternateSetting = 0x00;
			inter_dscrptr->bNumEndpoints = 0x02;
			inter_dscrptr->bInterfaceClass = USB_CLASS_MASS_STORAGE;//mass storage
			inter_dscrptr->bInterfaceSubClass = US_SC_SCSI;
			inter_dscrptr->bInterfaceProtocol = US_PR_BULK;
			inter_dscrptr->iInterface = 1;

			bytes_remaining -= inter_dscrptr->bLength;
			/* ep_in */
			ep_in->bLength = min(bytes_remaining, sizeof(struct usb_endpoint_descriptor));
			ep_in->bDescriptorType = USB_DT_ENDPOINT;
			ep_in->bEndpointAddress = sunxi_usb_get_ep_in_type(); /* IN */
			ep_in->bmAttributes = USB_ENDPOINT_XFER_BULK;
			ep_in->wMaxPacketSize = sunxi_usb_get_ep_max();
			ep_in->bInterval = 0x00;

			bytes_remaining -= ep_in->bLength;
			/* ep_out */
			ep_out->bLength = min(bytes_remaining, sizeof(struct usb_endpoint_descriptor));
			ep_out->bDescriptorType = USB_DT_ENDPOINT;
			ep_out->bEndpointAddress = sunxi_usb_get_ep_out_type(); /* OUT */
			ep_out->bmAttributes = USB_ENDPOINT_XFER_BULK;
			ep_out->wMaxPacketSize = sunxi_usb_get_ep_max();
			ep_out->bInterval = 0x00;

			bytes_remaining -= ep_out->bLength;

			sunxi_usb_send_setup(min(req->length, bytes_total), buffer);
		} break;
		case USB_DT_STRING: {
			uint8_t bLength = 0;
			uint8_t string_index = req->value & 0xff;

			printk_trace("USB MASS: get string descriptor\n");

			/* Language ID */
			if (string_index == 0) {
				bLength = min(4, req->length);

				sunxi_usb_send_setup(bLength, (void *) sunxi_usb_mass_dev[0]);
			} else if (string_index < SUNXI_USB_MASS_DEV_MAX) {
				/* Size of string in chars */
				uint8_t i = 0;
				uint8_t str_length = strlen((const char *) sunxi_usb_mass_dev[string_index]);
				uint8_t bLength = 2 + (2 * str_length);

				buffer[0] = bLength;	   /* length */
				buffer[1] = USB_DT_STRING; /* descriptor = string */

				/* Copy device string to fifo, expand to simple unicode */
				for (i = 0; i < str_length; i++) {
					buffer[2 + 2 * i + 0] = sunxi_usb_mass_dev[string_index][i];
					buffer[2 + 2 * i + 1] = 0;
				}
				bLength = min(bLength, req->length);
				sunxi_usb_send_setup(bLength, buffer);
			} else {
				printk_error("USB MASS: string line %d is not supported\n", string_index);
			}
		} break;

		case USB_DT_DEVICE_QUALIFIER: {
			struct usb_qualifier_descriptor *qua_dscrpt;
			printk_trace("USB MASS: get qualifier descriptor\n");
			qua_dscrpt = (struct usb_qualifier_descriptor *) buffer;
			memset(&buffer, 0, sizeof(struct usb_qualifier_descriptor));
			qua_dscrpt->bLength = min(req->length, sizeof(sizeof(struct usb_qualifier_descriptor)));
			qua_dscrpt->bDescriptorType = USB_DT_DEVICE_QUALIFIER;
			qua_dscrpt->bcdUSB = 0x200;
			qua_dscrpt->bDeviceClass = 0xff;
			qua_dscrpt->bDeviceSubClass = 0xff;
			qua_dscrpt->bDeviceProtocol = 0xff;
			qua_dscrpt->bMaxPacketSize0 = 0x40;
			qua_dscrpt->bNumConfigurations = 1;
			qua_dscrpt->breserved = 0;
			sunxi_usb_send_setup(qua_dscrpt->bLength, buffer);
		} break;
		default:
			printk_error("USB MASS: unkown value(%d)\n", req->value);

			ret = SUNXI_USB_REQ_OP_ERR;
	}

	return ret;
}

/**
 * usb_mass_usb_get_status - Handle a USB request to retrieve the status of the device
 *
 * This function handles a USB request to retrieve the status of the USB device. The status is returned in a buffer provided by the caller.
 *
 * @param req: Pointer to the USB device request structure
 * @param buffer: Pointer to the buffer to hold the status data
 *
 * @return: Error code indicating the result of the function
 *      SUNXI_USB_REQ_SUCCESSED if the request is successful
 *      SUNXI_USB_REQ_OP_ERR if there is an operational error
 */
static int usb_mass_usb_get_status(struct usb_device_request *req, uint8_t *buffer) {
	uint8_t bLength = 0;
	printk_trace("USB MASS: get status\n");
	if (0 == req->length) {
		/* sent zero packet */
		sunxi_usb_send_setup(0, NULL);
		return SUNXI_USB_REQ_OP_ERR;
	}
	bLength = min(req->value, 2);

	// Set the status information in the buffer
	buffer[0] = 1;// Device is busy
	buffer[1] = 0;// No error

	// Send the response back to the host
	sunxi_usb_send_setup(bLength, buffer);

	// Return the result of the operation
	return SUNXI_USB_REQ_SUCCESSED;
}

/**
 * sunxi_usb_mass_init - Initialize the USB Mass Storage driver
 *
 * This function initializes the USB Mass Storage driver by allocating memory for data transmission and setting up initial values.
 *
 * @return: 0 if initialization is successful, otherwise -1
 */
static int sunxi_usb_mass_init(void) {
	printk_trace("USB MASS: sunxi_mass_init\n");

	// Reset the transmission data structure
	memset(&trans_data, 0, sizeof(mass_trans_set_t));

	// Set initial values
	sunxi_usb_mass_write_enable = 0;
	sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;

	// Allocate memory for receive buffer
	trans_data.base_recv_buffer = (uint8_t *) smalloc(SUNXI_MASS_RECV_MEM_SIZE);
	if (!trans_data.base_recv_buffer) {
		printk_error("USB MASS: unable to malloc memory for mass receive\n");
		return -1;
	}

	// Allocate memory for send buffer
	trans_data.base_send_buffer = (uint8_t *) smalloc(SUNXI_MASS_SEND_MEM_SIZE);
	if (!trans_data.base_send_buffer) {
		printk_error("USB MASS: unable to malloc memory for mass send\n");
		sfree(trans_data.base_recv_buffer);
		return -1;
	}

	printk_trace("USB MASS: recv addr 0x%x\n", (uint32_t) trans_data.base_recv_buffer);
	printk_trace("USB MASS: send addr 0x%x\n", (uint32_t) trans_data.base_send_buffer);

	return 0;
}

/**
 * sunxi_mass_exit - Exit the USB Mass Storage driver
 *
 * This function frees the memory allocated for data transmission and exits the USB Mass Storage driver.
 *
 * @return: 0 if exit is successful, otherwise an error code
 */
static int sunxi_mass_exit(void) {
	printk_trace("USB MASS: sunxi_mass_exit\n");

	// Free receive buffer memory
	if (trans_data.base_recv_buffer) {
		sfree(trans_data.base_recv_buffer);
	}

	// Free send buffer memory
	if (trans_data.base_send_buffer) {
		sfree(trans_data.base_send_buffer);
	}

	return 0;
}

/**
 * sunxi_mass_reset - Reset the USB Mass Storage driver
 *
 * This function resets the USB Mass Storage driver by setting initial values.
 */
static void sunxi_mass_reset(void) {
	sunxi_usb_mass_write_enable = 0;
	sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;
}

/**
 * sunxi_mass_usb_rx_dma_isr - Handle the USB Mass Storage RX DMA interrupt
 *
 * This function is called when a USB Mass Storage RX DMA interrupt occurs. It sets a flag to indicate that the USB host is ready to write data.
 *
 * @param p_arg: Pointer to a void argument (not used)
 */
static void sunxi_mass_usb_rx_dma_isr(void *p_arg) {
	printk_trace("USB MASS: dma int for usb rx occur\n");
	sunxi_usb_mass_write_enable = 1;
}

/**
 * sunxi_mass_usb_tx_dma_isr - Handle the USB Mass Storage TX DMA interrupt
 *
 * This function is called when a USB Mass Storage TX DMA interrupt occurs. It currently does not perform any action.
 *
 * @param p_arg: Pointer to a void argument (not used)
 */
static void sunxi_mass_usb_tx_dma_isr(void *p_arg) {
	printk_trace("USB MASS: dma int for usb tx occur\n");
}
/**
 * sunxi_mass_standard_req_op - Handle standard USB Mass Storage requests
 *
 * This function handles standard USB Mass Storage requests by calling the corresponding functions based on the command.
 *
 * @param cmd: The command code for the request
 * @param req: Pointer to the USB device request structure
 * @param buffer: Pointer to the data buffer
 * @return: The result of the operation (error code or success)
 */
static int sunxi_mass_standard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer) {
	int ret = SUNXI_USB_REQ_OP_ERR;

	switch (cmd) {
		case USB_REQ_GET_STATUS: {
			ret = usb_mass_usb_get_status(req, buffer);
			break;
		}
		case USB_REQ_SET_ADDRESS: {
			ret = usb_mass_usb_set_address(req);
			break;
		}
		case USB_REQ_GET_DESCRIPTOR:
		case USB_REQ_GET_CONFIGURATION: {
			ret = usb_mass_usb_get_descriptor(req, buffer);
			break;
		}
		case USB_REQ_SET_CONFIGURATION: {
			ret = usb_mass_usb_set_configuration(req);
			break;
		}
		case USB_REQ_SET_INTERFACE: {
			ret = usb_mass_usb_set_interface(req);
			break;
		}
		default: {
			printk_error("USB MASS: standard req is not supported\n");
			ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
			break;
		}
	}
	return ret;
}

/**
 * sunxi_mass_nonstandard_req_op - Handle non-standard USB Mass Storage requests
 *
 * This function handles non-standard USB Mass Storage requests by calling the corresponding functions based on the request type and request code.
 *
 * @param cmd: The command code for the request
 * @param req: Pointer to the USB device request structure
 * @param buffer: Pointer to the data buffer
 * @param data_status: The status of the data transfer
 * @return: The result of the operation (error code or success)
 */
static int sunxi_mass_nonstandard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status) {
	int ret = SUNXI_USB_REQ_SUCCESSED;

	switch (req->request_type) {
		case 161:
			if (req->request == 0xFE) {
				printk_trace("USB MASS: mass ask for max lun\n");
				buffer[0] = 0;
				sunxi_usb_send_setup(1, buffer);
			} else {
				printk_error("USB MASS: unknown ep0 req in mass\n");
				ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
			}
			break;
		default:
			printk_error("USB MASS: unknown non standard ep0 req\n");
			ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
			break;
	}

	return ret;
}

/**
 * sunxi_mass_state_loop - State machine for USB Mass Storage operation
 *
 * This function implements a state machine for USB Mass Storage operation. It handles the different stages of the operation,
 * such as sending/receiving data, checking status, and handling errors.
 *
 * @param buffer: Pointer to the data buffer
 * @return: The result of the operation (error code or success)
 */
static int sunxi_mass_state_loop(void *buffer) {
	static struct umass_bbb_cbw_t *cbw;
	static struct umass_bbb_csw_t csw;
	static uint32_t mass_flash_start = 0;
	static uint32_t mass_flash_sectors = 0;
	int ret;
	sunxi_ubuf_t *sunxi_ubuf = (sunxi_ubuf_t *) buffer;

	switch (sunxi_usb_mass_status) {
		case SUNXI_USB_MASS_IDLE:
			if (sunxi_ubuf->rx_ready_for_data == 1) {
				sunxi_usb_mass_status = SUNXI_USB_MASS_SETUP;
			}
			break;
		case SUNXI_USB_MASS_SETUP:
			printk_trace("USB MASS: SUNXI_USB_MASS_SETUP\n");
			if (sunxi_ubuf->rx_req_length != sizeof(struct umass_bbb_cbw_t)) {
				printk_error("USB MASS: sunxi usb error: received bytes 0x%x is not equal cbw struct size 0x%x\n", sunxi_ubuf->rx_req_length, sizeof(struct umass_bbb_cbw_t));
				sunxi_ubuf->rx_ready_for_data = 0;
				sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;
				break;
			}

			cbw = (struct umass_bbb_cbw_t *) sunxi_ubuf->rx_req_buffer;
			if (CBWSIGNATURE != cbw->dCBWSignature) {
				printk_error("USB MASS: sunxi usb error: the cbw signature 0x%x is bad, need 0x%x\n", cbw->dCBWSignature, CBWSIGNATURE);
				sunxi_ubuf->rx_ready_for_data = 0;
				sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;
				break;
			}

			csw.dCSWSignature = CSWSIGNATURE;
			csw.dCSWTag = cbw->dCBWTag;

			printk_trace("USB MASS: usb cbw command = 0x%x\n", cbw->CBWCDB[0]);

			switch (cbw->CBWCDB[0]) {
				case SCSI_TST_U_RDY:
					printk_trace("USB MASS: SCSI_TST_U_RDY\n");
					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_STATUS;
					break;

				case SCSI_REQ_SENSE:
					printk_trace("USB MASS: SCSI_REQ_SENSE\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);

					trans_data.send_size = min(cbw->dCBWDataTransferLength, 18);
					trans_data.act_send_buffer = (uint32_t) request_sense;

					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_SEND_DATA;

					break;

				case SCSI_VERIFY:
					printk_trace("USB MASS: SCSI_VERIFY\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);

					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_STATUS;

					break;

				case SCSI_INQUIRY:
					printk_trace("USB MASS: SCSI_INQUIRY\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);

					trans_data.send_size = min(cbw->dCBWDataTransferLength, 36);
					trans_data.act_send_buffer = (uint32_t) inquiry_data;

					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_SEND_DATA;

					break;

				case SCSI_MODE_SEN6:
					printk_trace("USB MASS: SCSI_MODE_SEN6\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);
					{
						trans_data.base_send_buffer[0] = 3;
						trans_data.base_send_buffer[1] = 0;
						trans_data.base_send_buffer[2] = 0;
						trans_data.base_send_buffer[3] = 0;

						trans_data.act_send_buffer = (uint32_t) trans_data.base_send_buffer;
						trans_data.send_size = min(cbw->dCBWDataTransferLength, 4);
					}

					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_SEND_DATA;

					break;

				case SCSI_RD_CAPAC:
					printk_trace("USB MASS: SCSI_RD_CAPAC\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);
					{
						memset(trans_data.base_send_buffer, 0, 8);

						trans_data.base_send_buffer[2] = 0x80;
						trans_data.base_send_buffer[6] = 2;

						trans_data.act_send_buffer = (uint32_t) trans_data.base_send_buffer;
						trans_data.send_size = min(cbw->dCBWDataTransferLength, 8);
					}

					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_SEND_DATA;

					break;

				case SCSI_RD_FMT_CAPAC:
					printk_trace("USB MASS: SCSI_RD_FMT_CAPAC\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);
					{
						memset(trans_data.base_send_buffer, 0, 12);

						trans_data.base_send_buffer[2] = 0x80;
						trans_data.base_send_buffer[6] = 2;
						trans_data.base_send_buffer[8] = 2;
						trans_data.base_send_buffer[10] = 2;

						trans_data.act_send_buffer = (uint32_t) trans_data.base_send_buffer;
						trans_data.send_size = min(cbw->dCBWDataTransferLength, 12);
					}

					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_SEND_DATA;

					break;
				case SCSI_MED_REMOVL:
					printk_trace("USB MASS: SCSI_MED_REMOVL\n");
					csw.bCSWStatus = 0;
					sunxi_usb_mass_status = SUNXI_USB_MASS_STATUS;

					break;
				case SCSI_READ6:
				case SCSI_READ10://HOST READ, not this device read
					printk_trace("USB MASS: SCSI_READ\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);
					{
						uint32_t start, sectors;
						uint32_t offset;

						start = (cbw->CBWCDB[2] << 24) | cbw->CBWCDB[3] << 16 | cbw->CBWCDB[4] << 8 | cbw->CBWCDB[5] << 0;
						sectors = (cbw->CBWCDB[7] << 8) | cbw->CBWCDB[8];
						printk_trace("USB MASS: read start: 0x%x, sectors 0x%x\n", start, sectors);

						trans_data.send_size = min(cbw->dCBWDataTransferLength, sectors * 512);
						trans_data.act_send_buffer = (uint32_t) trans_data.base_send_buffer;
						if (!card0.online) {
							if (sdmmc_blk_read(&card0, trans_data.base_send_buffer, start, sectors) != sectors) {
								printk_error("USB MASS: sunxi flash read err: start,0x%x sectors 0x%x\n", start, sectors);
								csw.bCSWStatus = 1;
							} else {
								csw.bCSWStatus = 0;
								ret = 1;
							}
						} else {
							printk_error("USB MASS: sunxi flash read err: start,0x%x sectors 0x%x\n", start, sectors);
							csw.bCSWStatus = 1;
						}
						sunxi_usb_mass_status = SUNXI_USB_MASS_SEND_DATA;
					}

					break;

				case SCSI_WRITE6:
				case SCSI_WRITE10://HOST WRITE, not this device write
					printk_trace("USB MASS: SCSI_WRITE\n");
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);
					mass_flash_start = (cbw->CBWCDB[2] << 24) | cbw->CBWCDB[3] << 16 | cbw->CBWCDB[4] << 8 | cbw->CBWCDB[5] << 0;
					mass_flash_sectors = (cbw->CBWCDB[7] << 8) | cbw->CBWCDB[8];
					printk_trace("USB MASS: command write start: 0x%x, sectors 0x%x\n", mass_flash_start, mass_flash_sectors);
					trans_data.recv_size = min(cbw->dCBWDataTransferLength, mass_flash_sectors * 512);
					trans_data.act_recv_buffer = (uint32_t) trans_data.base_recv_buffer;
					// TODO Write function
					mass_flash_start += (0);
					printk_trace("USB MASS: try to receive data 0x%x\n", trans_data.recv_size);
					sunxi_usb_mass_write_enable = 0;
					sunxi_usb_start_recv_by_dma((void *) trans_data.act_recv_buffer, trans_data.recv_size);//start dma to receive data
					sunxi_usb_mass_status = SUNXI_USB_MASS_RECEIVE_DATA;
					break;
				default:
					printk_trace("USB MASS: not supported command 0x%x now\n", cbw->CBWCDB[0]);
					printk_trace("USB MASS: asked size 0x%x\n", cbw->dCBWDataTransferLength);
					csw.bCSWStatus = 1;
					sunxi_usb_mass_status = SUNXI_USB_MASS_STATUS;
					break;
			}
			break;
		case SUNXI_USB_MASS_SEND_DATA:
			printk_trace("USB MASS: SUNXI_USB_SEND_DATA\n");
			sunxi_usb_mass_status = SUNXI_USB_MASS_STATUS;
			sunxi_usb_send_data((void *) trans_data.act_send_buffer, trans_data.send_size);
			break;

		case SUNXI_USB_MASS_RECEIVE_DATA:
			printk_trace("USB MASS: SUNXI_USB_RECEIVE_DATA\n");
			// TODO Flash Write
			break;

		case SUNXI_USB_MASS_STATUS:
			printk_trace("USB MASS: SUNXI_USB_MASS_STATUS\n");
			{
				sunxi_usb_mass_status = SUNXI_USB_MASS_IDLE;
				sunxi_ubuf->rx_ready_for_data = 0;
				sunxi_usb_send_data((uint8_t *) &csw, sizeof(struct umass_bbb_csw_t));
			}

			break;
		default:
			break;
	}

	return 0;
}

sunxi_usb_module_init(SUNXI_USB_DEVICE_MASS, sunxi_usb_mass_init, sunxi_mass_exit, sunxi_mass_reset, sunxi_mass_standard_req_op, sunxi_mass_nonstandard_req_op,
					  sunxi_mass_state_loop, sunxi_mass_usb_rx_dma_isr, sunxi_mass_usb_tx_dma_isr);
