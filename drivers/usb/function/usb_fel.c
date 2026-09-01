/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <log.h>
#ifdef CONFIG_ARCH_DCACHE
#include <cache.h>
#endif

#include <drivers/usb/function/usb_fel.h>
#include <drivers/usb/function/usb_function.h>
#include <drivers/usb/usb_manager.h>
#include <drivers/soc/soc.h>

#define SUNXI_USB_FEL_MAX_TRANSFER	(64U * 1024U)
#define SUNXI_USB_FEL_PIO_TRANSFER_MAX	512U
#define SUNXI_USB_FEL_PHOENIX_DATA_SIZE (1U << SUNXI_FEL_PHOENIX_DATA_LEN_NR)

/* These are the data blocks used by the common FEL commands. */
struct sunxi_fel_is_ready_data_t {
	uint16_t state;
	uint16_t interval_ms;
	uint8_t reserved[12];
} __attribute__((packed));

struct sunxi_fel_cmd_set_ver_data_t {
	uint16_t ver_high;
	uint16_t ver_low;
	uint8_t reserved[12];
} __attribute__((packed));

enum sunxi_usb_fel_state {
	SUNXI_USB_FEL_WAIT_HEADER = 0,
	SUNXI_USB_FEL_WAIT_REQUEST,
	SUNXI_USB_FEL_WAIT_WRITE_DATA,
	SUNXI_USB_FEL_WAIT_WRITE_DMA,
};

static struct sunxi_usb_request_t sunxi_usb_fel_transport_request __attribute__((aligned(4)));
static struct sunxi_efex_request_t sunxi_usb_fel_request __attribute__((aligned(4)));
static struct sunxi_efex_response_t sunxi_usb_fel_status __attribute__((aligned(4)));
static struct sunxi_efex_device_resp_t sunxi_usb_fel_device_response;
static struct sunxi_fel_is_ready_data_t sunxi_usb_fel_ready_data __attribute__((aligned(4)));
static struct sunxi_fel_cmd_set_ver_data_t sunxi_usb_fel_version_data __attribute__((aligned(4)));
static uint8_t sunxi_usb_fel_phoenix_data[SUNXI_USB_FEL_PHOENIX_DATA_SIZE] __attribute__((aligned(4)));

static volatile enum sunxi_usb_fel_state sunxi_usb_fel_state;
static uint8_t *sunxi_usb_fel_read_buffer;
static uint32_t sunxi_usb_fel_read_length;
static uint8_t *sunxi_usb_fel_write_buffer;
static uint32_t sunxi_usb_fel_write_length;
static uint8_t sunxi_usb_fel_read_pending;
static uint8_t sunxi_usb_fel_status_pending;
static uint8_t sunxi_usb_fel_exec_pending;
static volatile uint8_t sunxi_usb_fel_dma_done;
static uint8_t sunxi_usb_fel_configuration;

_Static_assert(sizeof(struct sunxi_usb_request_t) == 32U, "invalid FEL transport request size");
_Static_assert(sizeof(struct sunxi_usb_response_t) == 13U, "invalid FEL transport response size");
_Static_assert(sizeof(struct sunxi_efex_request_t) == 16U, "invalid FEL request size");
_Static_assert(sizeof(struct sunxi_efex_response_t) == 8U, "invalid FEL response size");
_Static_assert(sizeof(struct sunxi_efex_device_resp_t) == 32U, "invalid FEL verify response size");

static bool sunxi_usb_fel_range_valid(uint32_t address, uint32_t length)
{
	if (address == 0U || length == 0U || length > SUNXI_USB_FEL_MAX_TRANSFER)
		return false;

	/* Addresses are wire-format uint32_t values, so reject wraparound. */
	return (length - 1U) <= 0xffffffffU - address;
}

static void sunxi_usb_fel_dma_invalidate(uint8_t *buffer, uint32_t length)
{
#ifdef CONFIG_ARCH_DCACHE
	if (buffer != NULL && length != 0U)
		invalidate_dcache_range((uint64_t)(uintptr_t)buffer, (uint64_t)(uintptr_t)buffer + length);
#else
	(void)buffer;
	(void)length;
#endif
}

static void sunxi_usb_fel_dma_flush(uint8_t *buffer, uint32_t length)
{
#ifdef CONFIG_ARCH_DCACHE
	if (buffer != NULL && length != 0U)
		flush_dcache_range((uint64_t)(uintptr_t)buffer, (uint64_t)(uintptr_t)buffer + length);
#else
	(void)buffer;
	(void)length;
#endif
}

static void sunxi_usb_fel_set_status(uint16_t tag, uint8_t status)
{
	memset(&sunxi_usb_fel_status, 0, sizeof(sunxi_usb_fel_status));
	sunxi_usb_fel_status.magic = SUNXI_EFEX_STATUS_CMD;
	sunxi_usb_fel_status.tag = tag;
	sunxi_usb_fel_status.status = status;
	sunxi_usb_fel_status_pending = 1U;
}

static int sunxi_usb_fel_send_transport_response(uint32_t tag, uint32_t residue, uint8_t status)
{
	struct sunxi_usb_response_t response __attribute__((aligned(4)));

	memset(&response, 0, sizeof(response));
	memcpy(response.magic, SUNXI_USB_RSP_MAGIC, sizeof(response.magic));
	response.tag = tag;
	response.residue = residue;
	response.status = status;
	return sunxi_usb_send_data(&response, sizeof(response));
}

/* libefex reads the application status as a separate, unframed packet. */
static int sunxi_usb_fel_send_status(void)
{
	int ret;

	if (!sunxi_usb_fel_status_pending)
		return 0;

	ret = sunxi_usb_send_data(&sunxi_usb_fel_status, sizeof(sunxi_usb_fel_status));
	if (ret == 0)
		sunxi_usb_fel_status_pending = 0U;
	return ret;
}

static void sunxi_usb_fel_clear_command(void)
{
	sunxi_usb_fel_read_buffer = NULL;
	sunxi_usb_fel_read_length = 0U;
	sunxi_usb_fel_write_buffer = NULL;
	sunxi_usb_fel_write_length = 0U;
	sunxi_usb_fel_read_pending = 0U;
	sunxi_usb_fel_status_pending = 0U;
	sunxi_usb_fel_exec_pending = 0U;
	sunxi_usb_fel_dma_done = 0U;
}

static void sunxi_usb_fel_prepare_verify_response(void)
{
	memset(&sunxi_usb_fel_device_response, 0, sizeof(sunxi_usb_fel_device_response));
	memcpy(sunxi_usb_fel_device_response.magic, SUNXI_VERIFY_RSP_MAGIC,
		sizeof(sunxi_usb_fel_device_response.magic));
	sunxi_usb_fel_device_response.id = sunxi_soc_platform_id() >> 8;
	sunxi_usb_fel_device_response.firmware = 1U;
	sunxi_usb_fel_device_response.mode = DEVICE_MODE_FEL;
	sunxi_usb_fel_device_response.data_flag = 'D';
	sunxi_usb_fel_device_response.data_length = SUNXI_FEL_PHOENIX_DATA_LEN_NR;
	sunxi_usb_fel_device_response.data_start_address = (uint32_t)(uintptr_t)sunxi_usb_fel_phoenix_data;

	sunxi_usb_fel_read_buffer = (uint8_t *)&sunxi_usb_fel_device_response;
	sunxi_usb_fel_read_length = sizeof(sunxi_usb_fel_device_response);
	sunxi_usb_fel_read_pending = 1U;
}

/* Decode and stage one libefex sunxi_efex_request_t. */
static uint8_t sunxi_usb_fel_process_request(const struct sunxi_efex_request_t *request)
{
	uint16_t command;

	memcpy(&sunxi_usb_fel_request, request, sizeof(sunxi_usb_fel_request));
	command = sunxi_usb_fel_request.cmd;

	/* Clear response state before staging the new command. */
	sunxi_usb_fel_read_pending = 0U;
	sunxi_usb_fel_status_pending = 0U;
	sunxi_usb_fel_exec_pending = 0U;

	switch (command) {
	case EFEX_CMD_VERIFY_DEVICE:
		sunxi_usb_fel_prepare_verify_response();
		sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, 0U);
		return 0U;

	case EFEX_CMD_IS_READY:
		memset(&sunxi_usb_fel_ready_data, 0, sizeof(sunxi_usb_fel_ready_data));
		sunxi_usb_fel_ready_data.state = 2U; /* AL_IS_READY_STATE_READY */
		sunxi_usb_fel_ready_data.interval_ms = 500U;
		sunxi_usb_fel_read_buffer = (uint8_t *)&sunxi_usb_fel_ready_data;
		sunxi_usb_fel_read_length = sizeof(sunxi_usb_fel_ready_data);
		sunxi_usb_fel_read_pending = 1U;
		sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, 0U);
		return 0U;

	case EFEX_CMD_GET_CMD_SET_VER:
		memset(&sunxi_usb_fel_version_data, 0, sizeof(sunxi_usb_fel_version_data));
		sunxi_usb_fel_version_data.ver_high = 2U;
		sunxi_usb_fel_version_data.ver_low = 0U;
		sunxi_usb_fel_read_buffer = (uint8_t *)&sunxi_usb_fel_version_data;
		sunxi_usb_fel_read_length = sizeof(sunxi_usb_fel_version_data);
		sunxi_usb_fel_read_pending = 1U;
		sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, 0U);
		return 0U;

	case EFEX_CMD_SWITCH_ROLE:
	case EFEX_CMD_DISCONNECT:
		/* These operations are intentionally unsupported in FEL mode. */
		break;

	case EFEX_CMD_FEL_READ:
		if (!sunxi_usb_fel_range_valid(sunxi_usb_fel_request.address, sunxi_usb_fel_request.len))
			break;
		sunxi_usb_fel_read_buffer = (uint8_t *)(uintptr_t)sunxi_usb_fel_request.address;
		sunxi_usb_fel_read_length = sunxi_usb_fel_request.len;
		sunxi_usb_fel_read_pending = 1U;
		sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, 0U);
		return 0U;

	case EFEX_CMD_FEL_WRITE:
		if (!sunxi_usb_fel_range_valid(sunxi_usb_fel_request.address, sunxi_usb_fel_request.len))
			break;
		sunxi_usb_fel_write_buffer = (uint8_t *)(uintptr_t)sunxi_usb_fel_request.address;
		sunxi_usb_fel_write_length = sunxi_usb_fel_request.len;
		return 0U;

	case EFEX_CMD_FEL_EXEC:
		if (sunxi_usb_fel_request.address == 0U || sunxi_usb_fel_request.len != 0U)
			break;
		sunxi_usb_fel_exec_pending = 1U;
		sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, 0U);
		return 0U;

	default:
		break;
	}

	sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, 1U);
	return 1U;
}

static void sunxi_usb_fel_finish_write(sunxi_ubuf_t *sunxi_ubuf, uint8_t transport_status)
{
	uint8_t status = transport_status;

	if (status == 0U && sunxi_ubuf->rx_req_length != sunxi_usb_fel_write_length) {
		printk_error("USB FEL: write length %u, expected %u\n", sunxi_ubuf->rx_req_length,
			sunxi_usb_fel_write_length);
		status = 1U;
	}
	if (status == 0U)
		memmove(sunxi_usb_fel_write_buffer, sunxi_ubuf->rx_req_buffer, sunxi_usb_fel_write_length);

	sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, status);
	sunxi_usb_fel_write_buffer = NULL;
	sunxi_usb_fel_write_length = 0U;
	sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
	(void)sunxi_usb_fel_send_transport_response(sunxi_usb_fel_transport_request.tab, 0U, status);
}

static void sunxi_usb_fel_finish_dma_write(void)
{
	uint8_t status = sunxi_usb_fel_dma_done ? 0U : 1U;

	if (status != 0U)
		printk_error("USB FEL: DMA write did not complete\n");
	if (status == 0U)
		sunxi_usb_fel_dma_invalidate(sunxi_usb_fel_write_buffer, sunxi_usb_fel_write_length);
	sunxi_usb_fel_set_status(sunxi_usb_fel_request.tag, status);
	sunxi_usb_fel_write_buffer = NULL;
	sunxi_usb_fel_write_length = 0U;
	sunxi_usb_fel_dma_done = 0U;
	sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
	(void)sunxi_usb_fel_send_transport_response(sunxi_usb_fel_transport_request.tab, 0U, status);
}

static void sunxi_usb_fel_handle_header(sunxi_ubuf_t *sunxi_ubuf)
{
	uint32_t length;
	uint32_t pio_limit;
	uint32_t transport_tag;
	uint8_t direction;
	uint8_t status = 0U;
	bool execute = false;
	void (*entry)(void) = NULL;
	int response_status;

	memset(&sunxi_usb_fel_transport_request, 0, sizeof(sunxi_usb_fel_transport_request));
	memcpy(&sunxi_usb_fel_transport_request, sunxi_ubuf->rx_req_buffer,
		min(sunxi_ubuf->rx_req_length, (uint32_t)sizeof(sunxi_usb_fel_transport_request)));
	sunxi_ubuf->rx_ready_for_data = 0U;
	transport_tag = sunxi_usb_fel_transport_request.tab;

	if (sunxi_ubuf->rx_req_length != sizeof(struct sunxi_usb_request_t) ||
		memcmp(sunxi_usb_fel_transport_request.magic, SUNXI_USB_REQ_MAGIC, 4U) != 0) {
		printk_error("USB FEL: invalid AWUC request\n");
		sunxi_usb_fel_clear_command();
		(void)sunxi_usb_fel_send_transport_response(transport_tag, 0U, 1U);
		return;
	}

	length = sunxi_usb_fel_transport_request.data_length;
	direction = sunxi_usb_fel_transport_request.cmd_package[0];
	pio_limit = (uint32_t)sunxi_usb_get_ep_max();
	if (pio_limit == 0U || pio_limit > SUNXI_USB_FEL_PIO_TRANSFER_MAX)
		pio_limit = SUNXI_USB_FEL_PIO_TRANSFER_MAX;
	if (length > SUNXI_USB_FEL_MAX_TRANSFER) {
		sunxi_usb_fel_clear_command();
		(void)sunxi_usb_fel_send_transport_response(transport_tag, length - SUNXI_USB_FEL_MAX_TRANSFER, 1U);
		return;
	}

	if (direction == AW_USB_WRITE) {
		if (sunxi_usb_fel_write_buffer != NULL) {
			if (length != sunxi_usb_fel_write_length) {
				status = 1U;
			} else if (length > pio_limit) {
				/* Clear the completion flag before arming DMA to avoid a fast-transfer race. */
				sunxi_usb_fel_dma_done = 0U;
				sunxi_usb_fel_dma_flush(sunxi_usb_fel_write_buffer, length);
				sunxi_usb_fel_dma_invalidate(sunxi_usb_fel_write_buffer, length);
				if (sunxi_usb_start_recv_by_dma(sunxi_usb_fel_write_buffer, length) != 0) {
					status = 1U;
				} else {
					sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_WRITE_DMA;
					return;
				}
			} else {
				sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_WRITE_DATA;
				return;
			}
		} else if (length == sizeof(struct sunxi_efex_request_t)) {
			sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_REQUEST;
			return;
		} else {
			status = 1U;
		}
	} else if (direction == AW_USB_READ) {
		/* A memory read of eight bytes must win over the status read below. */
		if (sunxi_usb_fel_read_pending && length == sunxi_usb_fel_read_length) {
			sunxi_usb_fel_dma_flush(sunxi_usb_fel_read_buffer, length);
			if (sunxi_usb_send_data(sunxi_usb_fel_read_buffer, length) != 0U)
				status = 1U;
			sunxi_usb_fel_read_pending = 0U;
			/* Application status is requested as a separate eight-byte read. */
		} else if (sunxi_usb_fel_status_pending && length == sizeof(sunxi_usb_fel_status)) {
			if (sunxi_usb_fel_send_status() != 0)
				status = 1U;
			else if (sunxi_usb_fel_exec_pending) {
				execute = true;
				entry = (void (*)(void))(uintptr_t)sunxi_usb_fel_request.address;
			}
		} else {
			status = 1U;
		}
	} else {
		status = 1U;
	}
	if (status != 0U) {
		sunxi_usb_fel_write_buffer = NULL;
		sunxi_usb_fel_write_length = 0U;
		sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
	}

	response_status = sunxi_usb_fel_send_transport_response(sunxi_usb_fel_transport_request.tab, 0U, status);
	if (response_status == 0 && status == 0U && execute) {
		sunxi_usb_fel_exec_pending = 0U;
		entry();
	}
}

static int sunxi_usb_fel_init(void)
{
	memset(&sunxi_usb_fel_device_response, 0, sizeof(sunxi_usb_fel_device_response));
	memset(sunxi_usb_fel_phoenix_data, 0xcc, sizeof(sunxi_usb_fel_phoenix_data));
	sunxi_usb_fel_clear_command();
	sunxi_usb_fel_configuration = 0U;
	sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
	return 0;
}

static int sunxi_usb_fel_exit(void)
{
	sunxi_usb_fel_clear_command();
	sunxi_usb_fel_configuration = 0U;
	sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
	return 0;
}

static void sunxi_usb_fel_reset(void)
{
	sunxi_usb_fel_clear_command();
	sunxi_usb_fel_configuration = 0U;
	sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
}

static int sunxi_usb_fel_send_string(const struct usb_device_request *req, uint8_t *buffer)
{
	static const char *const strings[] = { "Allwinner", "SyterKit FEL", "0001" };
	uint8_t index = req->value & 0xffU;
	uint32_t length = 2U;
	uint32_t character;

	if (index == 0U) {
		static const uint8_t language[] = { 4U, USB_DT_STRING, 0x09U, 0x04U };

		sunxi_usb_send_setup(min(req->length, sizeof(language)), (void *)language);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	if (index > sizeof(strings) / sizeof(strings[0]))
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;

	for (character = 0U; strings[index - 1U][character] != '\0' && length + 2U <= 255U; character++) {
		buffer[length++] = (uint8_t)strings[index - 1U][character];
		buffer[length++] = 0U;
	}
	buffer[0] = (uint8_t)length;
	buffer[1] = USB_DT_STRING;
	sunxi_usb_send_setup(min(req->length, length), buffer);
	return SUNXI_USB_REQ_SUCCESSED;
}

static int sunxi_usb_fel_get_descriptor(const struct usb_device_request *req, uint8_t *buffer)
{
	static const struct usb_device_descriptor device = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DT_DEVICE,
		.bcdUSB = 0x0200,
		.bMaxPacketSize0 = 64,
		.idVendor = SUNXI_USB_VENDOR,
		.idProduct = SUNXI_USB_PRODUCT,
		.bcdDevice = 0xffff,
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	};
	struct sunxi_usb_fel_configuration {
		struct usb_configuration_descriptor configuration;
		struct usb_interface_descriptor interface;
		struct usb_endpoint_descriptor ep_in;
		struct usb_endpoint_descriptor ep_out;
	} __attribute__((packed));
	struct sunxi_usb_fel_configuration configuration = {
		.configuration = {
			.bLength = sizeof(struct usb_configuration_descriptor),
			.bDescriptorType = USB_DT_CONFIG,
			.wTotalLength = sizeof(struct sunxi_usb_fel_configuration),
			.bNumInterfaces = 1,
			.bConfigurationValue = 1,
			.bmAttributes = 0x80,
			.bMaxPower = 150,
		},
		.interface = {
			.bLength = sizeof(struct usb_interface_descriptor),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
			.bInterfaceSubClass = 0xff,
			.bInterfaceProtocol = 0xff,
		},
		.ep_in = {
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 0x81,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = 512,
		},
		.ep_out = {
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 0x02,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = 512,
		},
	};
	uint8_t type = req->value >> 8;
	uint32_t length;

	switch (type) {
	case USB_DT_DEVICE:
		length = min(req->length, sizeof(device));
		sunxi_usb_send_setup(length, (void *)&device);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_DT_CONFIG:
		if ((req->value & 0xffU) != 0U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		configuration.ep_in.wMaxPacketSize = sunxi_usb_get_ep_max();
		configuration.ep_out.wMaxPacketSize = sunxi_usb_get_ep_max();
		length = min(req->length, sizeof(configuration));
		sunxi_usb_send_setup(length, &configuration);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_DT_OTHER_SPEED_CONFIG:
		if ((req->value & 0xffU) != 0U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		configuration.configuration.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
		configuration.ep_in.wMaxPacketSize = sunxi_usb_get_ep_max() == 64 ? 512 : 64;
		configuration.ep_out.wMaxPacketSize = configuration.ep_in.wMaxPacketSize;
		length = min(req->length, sizeof(configuration));
		sunxi_usb_send_setup(length, &configuration);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_DT_STRING:
		return sunxi_usb_fel_send_string(req, buffer);
	case USB_DT_DEVICE_QUALIFIER: {
		struct usb_qualifier_descriptor qualifier;

		memset(&qualifier, 0, sizeof(qualifier));
		qualifier.bLength = sizeof(qualifier);
		qualifier.bDescriptorType = USB_DT_DEVICE_QUALIFIER;
		qualifier.bcdUSB = 0x0200;
		qualifier.bDeviceClass = USB_CLASS_VENDOR_SPEC;
		qualifier.bDeviceSubClass = 0xff;
		qualifier.bDeviceProtocol = 0xff;
		qualifier.bMaxPacketSize0 = 64;
		qualifier.bNumConfigurations = 1;
		sunxi_usb_send_setup(min(req->length, sizeof(qualifier)), &qualifier);
		return SUNXI_USB_REQ_SUCCESSED;
	}
	default:
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
	}
}

static int sunxi_usb_fel_standard_req_op(uint32_t cmd, struct usb_device_request *req, uint8_t *buffer)
{
	if (req == NULL)
		return SUNXI_USB_REQ_OP_ERR;

	switch (cmd) {
	case USB_REQ_GET_STATUS:
		if (buffer == NULL)
			return SUNXI_USB_REQ_OP_ERR;
		buffer[0] = 0U;
		buffer[1] = 0U;
		sunxi_usb_send_setup(min(req->length, 2U), buffer);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_REQ_SET_ADDRESS:
		return sunxi_usb_set_address(req->value & 0x7fU);
	case USB_REQ_GET_DESCRIPTOR:
		return buffer == NULL ? SUNXI_USB_REQ_OP_ERR : sunxi_usb_fel_get_descriptor(req, buffer);
	case USB_REQ_GET_CONFIGURATION:
		if (buffer == NULL)
			return SUNXI_USB_REQ_OP_ERR;
		buffer[0] = sunxi_usb_fel_configuration;
		sunxi_usb_send_setup(min(req->length, 1U), buffer);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_REQ_SET_CONFIGURATION:
		if (req->value > 1U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		sunxi_usb_fel_configuration = (uint8_t)req->value;
		if (req->value == 1U)
			sunxi_usb_ep_reset();
		sunxi_usb_send_setup(0U, NULL);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_REQ_GET_INTERFACE:
		if (buffer == NULL || req->index != 0U)
			return SUNXI_USB_REQ_OP_ERR;
		buffer[0] = 0U;
		sunxi_usb_send_setup(min(req->length, 1U), buffer);
		return SUNXI_USB_REQ_SUCCESSED;
	case USB_REQ_SET_INTERFACE:
		if (req->index != 0U || req->value != 0U)
			return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
		sunxi_usb_ep_reset();
		sunxi_usb_send_setup(0U, NULL);
		return SUNXI_USB_REQ_SUCCESSED;
	default:
		return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
	}
}

static int sunxi_usb_fel_nonstandard_req_op(
	uint32_t cmd, struct usb_device_request *req, uint8_t *buffer, uint32_t data_status)
{
	(void)cmd;
	(void)req;
	(void)buffer;
	(void)data_status;
	return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
}

static int sunxi_usb_fel_state_loop(void *buffer)
{
	sunxi_ubuf_t *sunxi_ubuf = (sunxi_ubuf_t *)buffer;

	if (sunxi_ubuf == NULL)
		return SUNXI_USB_REQ_OP_ERR;

	switch (sunxi_usb_fel_state) {
	case SUNXI_USB_FEL_WAIT_HEADER:
		if (sunxi_ubuf->rx_ready_for_data)
			sunxi_usb_fel_handle_header(sunxi_ubuf);
		break;
	case SUNXI_USB_FEL_WAIT_REQUEST:
		if (sunxi_ubuf->rx_ready_for_data) {
			uint8_t status;

			if (sunxi_ubuf->rx_req_length != sizeof(struct sunxi_efex_request_t)) {
				status = 1U;
			} else {
				status = sunxi_usb_fel_process_request(
					(const struct sunxi_efex_request_t *)sunxi_ubuf->rx_req_buffer);
			}
			sunxi_ubuf->rx_ready_for_data = 0U;
			sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
			(void)sunxi_usb_fel_send_transport_response(sunxi_usb_fel_transport_request.tab, 0U, status);
		}
		break;
	case SUNXI_USB_FEL_WAIT_WRITE_DATA:
		if (sunxi_ubuf->rx_ready_for_data) {
			sunxi_ubuf->rx_ready_for_data = 0U;
			sunxi_usb_fel_finish_write(sunxi_ubuf, 0U);
		}
		break;
	case SUNXI_USB_FEL_WAIT_WRITE_DMA:
		if (sunxi_usb_fel_dma_done)
			sunxi_usb_fel_finish_dma_write();
		break;
	default:
		sunxi_usb_fel_state = SUNXI_USB_FEL_WAIT_HEADER;
		break;
	}

	return 0;
}

static void sunxi_usb_fel_rx_dma_isr(void *p_arg)
{
	(void)p_arg;
	sunxi_usb_fel_dma_done = 1U;
}

static void sunxi_usb_fel_tx_dma_isr(void *p_arg)
{
	(void)p_arg;
}

static const sunxi_usb_function_ops_t sunxi_usb_fel_ops = {
	.state_init = sunxi_usb_fel_init,
	.state_exit = sunxi_usb_fel_exit,
	.state_reset = sunxi_usb_fel_reset,
	.standard_req_op = sunxi_usb_fel_standard_req_op,
	.nonstandard_req_op = sunxi_usb_fel_nonstandard_req_op,
	.state_loop = sunxi_usb_fel_state_loop,
	.dma_rx_isr = sunxi_usb_fel_rx_dma_isr,
	.dma_tx_isr = sunxi_usb_fel_tx_dma_isr,
};

const sunxi_usb_function_t sunxi_usb_function_fel = {
	.type = SUNXI_USB_DEVICE_FEL,
	.name = "fel",
	.ops = &sunxi_usb_fel_ops,
};
