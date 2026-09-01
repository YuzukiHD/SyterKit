/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <drivers/clk/clk.h>

#include <drivers/usb/function/usb_function.h>
#include <drivers/usb/usb_manager.h>

#define WINUSB_MS_OS_STRING_INDEX  0xeeU
#define WINUSB_MS_VENDOR_CODE	   0x20U
#define WINUSB_EXT_COMPAT_ID_INDEX 0x0004U

volatile uint32_t sunxi_usb_winusb_flag;
static uint8_t sunxi_usb_winusb_configuration;

struct sunxi_usb_winusb_configuration {
	struct usb_configuration_descriptor configuration;
	struct usb_interface_descriptor interface;
} __attribute__((packed));

static const struct usb_device_descriptor sunxi_usb_winusb_device_descriptor = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x7d4a,
	.idProduct = 0x2b82,
	.bcdDevice = 0x0002,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct sunxi_usb_winusb_configuration sunxi_usb_winusb_config_descriptor = {
	.configuration = {
		.bLength = sizeof(struct usb_configuration_descriptor),
		.bDescriptorType = USB_DT_CONFIG,
		.wTotalLength = sizeof(struct sunxi_usb_winusb_configuration),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.bmAttributes = 0x80,
		.bMaxPower = 50,
	},
	.interface = {
		.bLength = sizeof(struct usb_interface_descriptor),
		.bDescriptorType = USB_DT_INTERFACE,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
	},
};

static const char *const sunxi_usb_winusb_strings[] = {
	"Allwinner",
	"SyterKit WinUSB Device",
	"0001",
};

/* Microsoft OS 1.0 string descriptor: "MSFT100" plus the vendor request code. */
static const uint8_t sunxi_usb_winusb_ms_os_string[] = {
	18U,
	USB_DT_STRING,
	'M',
	0U,
	'S',
	0U,
	'F',
	0U,
	'T',
	0U,
	'1',
	0U,
	'0',
	0U,
	'0',
	0U,
	WINUSB_MS_VENDOR_CODE,
	0U,
};

/* Microsoft Extended Compatible ID descriptor for interface 0. */
static const uint8_t sunxi_usb_winusb_compat_id[] = {
	0x28U,
	0x00U,
	0x00U,
	0x00U, /* dwLength */
	0x00U,
	0x01U, /* bcdVersion 1.00 */
	0x04U,
	0x00U, /* wIndex */
	0x01U, /* bCount */
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x01U, /* interface 0, reserved */
	'W',
	'I',
	'N',
	'U',
	'S',
	'B',
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
	0x00U,
};

_Static_assert(sizeof(sunxi_usb_winusb_ms_os_string) == 18U, "invalid Microsoft OS string descriptor");
_Static_assert(sizeof(sunxi_usb_winusb_compat_id) == 40U, "invalid WinUSB compatible ID descriptor");

static int sunxi_usb_winusb_send_string(const struct usb_device_request *req, uint8_t *buffer)
{
	uint8_t index = req->value & 0xffU;
	uint32_t character;
	uint32_t length;

	if (index == 0U) {
		static const uint8_t language[] = { 4U, USB_DT_STRING, 0x09U, 0x04U };

		sunxi_usb_send_setup(min(req->length, sizeof(language)), (void *)language);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	if (index == WINUSB_MS_OS_STRING_INDEX) {
		sunxi_usb_send_setup(
			min(req->length, sizeof(sunxi_usb_winusb_ms_os_string)), (void *)sunxi_usb_winusb_ms_os_string);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	if (index > ARRAY_SIZE(sunxi_usb_winusb_strings))
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;

	length = 2U;
	character = 0U;
	while (sunxi_usb_winusb_strings[index - 1U][character] != '\0' && length + 2U <= 255U) {
		buffer[length++] = (uint8_t)sunxi_usb_winusb_strings[index - 1U][character++];
		buffer[length++] = 0U;
	}
	buffer[0] = (uint8_t)length;
	buffer[1] = USB_DT_STRING;
	sunxi_usb_send_setup(min(req->length, length), buffer);
	return SUNXI_USB_REQ_SUCCESSED;
}

static int sunxi_usb_winusb_get_descriptor(const struct usb_device_request *req, uint8_t *buffer)
{
	uint8_t type = req->value >> 8;
	uint32_t length;

	switch (type) {
	case USB_DT_DEVICE:
		length = min(req->length, sizeof(sunxi_usb_winusb_device_descriptor));
		sunxi_usb_send_setup(length, (void *)&sunxi_usb_winusb_device_descriptor);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_DT_CONFIG:
		if ((req->value & 0xffU) != 0U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		length = min(req->length, sizeof(sunxi_usb_winusb_config_descriptor));
		sunxi_usb_send_setup(length, (void *)&sunxi_usb_winusb_config_descriptor);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_DT_OTHER_SPEED_CONFIG: {
		struct sunxi_usb_winusb_configuration *configuration = (struct sunxi_usb_winusb_configuration *)buffer;

		if ((req->value & 0xffU) != 0U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		memcpy(configuration, &sunxi_usb_winusb_config_descriptor, sizeof(*configuration));
		configuration->configuration.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
		length = min(req->length, sizeof(*configuration));
		sunxi_usb_send_setup(length, configuration);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	case USB_DT_STRING:
		return sunxi_usb_winusb_send_string(req, buffer);
	case USB_DT_DEVICE_QUALIFIER: {
		struct usb_qualifier_descriptor *qualifier = (struct usb_qualifier_descriptor *)buffer;

		memset(qualifier, 0, sizeof(*qualifier));
		qualifier->bLength = sizeof(*qualifier);
		qualifier->bDescriptorType = USB_DT_DEVICE_QUALIFIER;
		qualifier->bcdUSB = 0x0200;
		qualifier->bMaxPacketSize0 = 64;
		qualifier->bNumConfigurations = 1;
		sunxi_usb_send_setup(min(req->length, sizeof(*qualifier)), qualifier);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	default:
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
	}
}

static int sunxi_usb_winusb_init(void)
{
	printk_debug("USB: sunxi_usb_winusb_init\n");
	sunxi_usb_winusb_flag = 0;
	sunxi_usb_winusb_configuration = 0;
	return 0;
}

static int sunxi_usb_winusb_exit(void)
{
	printk_debug("USB: sunxi_usb_winusb_exit\n");
	sunxi_usb_winusb_flag = 0;
	sunxi_usb_winusb_configuration = 0;
	return 0;
}

static void sunxi_usb_winusb_reset(void)
{
	sunxi_usb_winusb_flag = 0;
	sunxi_usb_winusb_configuration = 0;
}

static void sunxi_usb_winusb_rx_dma_isr(void *p_arg)
{
	(void)p_arg;
	printk_debug("USB: dma int for usb rx occur\n");
}

static void sunxi_usb_winusb_tx_dma_isr(void *p_arg)
{
	(void)p_arg;
	printk_debug("USB: dma int for usb tx occur\n");
}

static int sunxi_usb_winusb_standard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer)
{
	if (req == NULL)
		return SUNXI_USB_REQ_OP_ERR;

	printk_trace("USB: sunxi_usb_winusb_standard_req_op get cmd = %d\n", cmd);
	switch (cmd) {
	case USB_REQ_GET_STATUS: {
		if (buffer == NULL)
			return SUNXI_USB_REQ_OP_ERR;
		buffer[0] = 0;
		buffer[1] = 0;
		sunxi_usb_send_setup(min(req->length, 2U), buffer);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	case USB_REQ_SET_ADDRESS: {
		return sunxi_usb_set_address(req->value & 0x7fU);
	}
	case USB_REQ_GET_DESCRIPTOR: {
		if (buffer == NULL)
			return SUNXI_USB_REQ_OP_ERR;
		return sunxi_usb_winusb_get_descriptor(req, buffer);
	}
	case USB_REQ_GET_CONFIGURATION: {
		if (buffer == NULL)
			return SUNXI_USB_REQ_OP_ERR;
		buffer[0] = sunxi_usb_winusb_configuration;
		sunxi_usb_send_setup(min(req->length, 1U), buffer);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	case USB_REQ_SET_CONFIGURATION: {
		if (req->value > 1U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		sunxi_usb_winusb_configuration = (uint8_t)req->value;
		sunxi_usb_winusb_flag = sunxi_usb_winusb_configuration;
		sunxi_usb_send_setup(0U, NULL);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	case USB_REQ_GET_INTERFACE: {
		if (buffer == NULL)
			return SUNXI_USB_REQ_OP_ERR;
		buffer[0] = 0;
		sunxi_usb_send_setup(min(req->length, 1U), buffer);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	case USB_REQ_SET_INTERFACE: {
		if (req->value != 0U || req->index != 0U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		sunxi_usb_send_setup(0U, NULL);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	default: {
		printk_error("USB: WinUSB standard request is not supported\n");
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
	}
	}
}

static int sunxi_usb_winusb_nonstandard_req_op(
	uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status)
{
	(void)cmd;
	(void)buffer;
	(void)data_status;

	if (req == NULL)
		return SUNXI_USB_REQ_OP_ERR;
	if (req->request_type != (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE) ||
		req->request != WINUSB_MS_VENDOR_CODE || req->value != 0U || req->index != WINUSB_EXT_COMPAT_ID_INDEX)
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;

	sunxi_usb_send_setup(min(req->length, sizeof(sunxi_usb_winusb_compat_id)), (void *)sunxi_usb_winusb_compat_id);
	return SUNXI_USB_REQ_SUCCESSED;
}

static int sunxi_usb_winusb_state_loop(void *buffer)
{
	(void)buffer;

	if (sunxi_usb_winusb_flag == 1U) {
		printk_info("USB: WinUSB host detected\n");
		sunxi_usb_winusb_flag = 2U;
	}
	return 0;
}

static const sunxi_usb_function_ops_t sunxi_usb_winusb_ops = {
	.state_init = sunxi_usb_winusb_init,
	.state_exit = sunxi_usb_winusb_exit,
	.state_reset = sunxi_usb_winusb_reset,
	.standard_req_op = sunxi_usb_winusb_standard_req_op,
	.nonstandard_req_op = sunxi_usb_winusb_nonstandard_req_op,
	.state_loop = sunxi_usb_winusb_state_loop,
	.dma_rx_isr = sunxi_usb_winusb_rx_dma_isr,
	.dma_tx_isr = sunxi_usb_winusb_tx_dma_isr,
};

const sunxi_usb_function_t sunxi_usb_function_winusb = {
	.type = SUNXI_USB_DEVICE_WINUSB,
	.name = "winusb",
	.ops = &sunxi_usb_winusb_ops,
};
