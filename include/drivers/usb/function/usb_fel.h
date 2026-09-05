/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_USB_FUNCTION_USB_FEL_H__
#define __DRIVERS_USB_FUNCTION_USB_FEL_H__

#include <stdint.h>

/** @brief Magic values used by the original AWUC/AWUS bulk transport. */
#define SUNXI_USB_REQ_MAGIC "AWUC"
#define SUNXI_USB_RSP_MAGIC "AWUS"

/** @brief USB vendor and product identifiers used by FEL. */
enum sunxi_usb_ids {
	SUNXI_USB_VENDOR = 0x1f3a,
	SUNXI_USB_PRODUCT = 0xefe8,
};

/** @brief FEL transport request codes. */
enum sunxi_efex_usb_request_t {
	AW_USB_READ = 0x11,
	AW_USB_WRITE = 0x12,
};

/** @brief FEL command status request marker. */
#define SUNXI_EFEX_STATUS_CMD 0xffffU

/** @brief FEL verification response constants and wire length. */
#define SUNXI_VERIFY_RSP_MAGIC	      "AWUSBFEX"
#define SUNXI_FEL_PHOENIX_DATA_LEN_NR 8U

/** @brief Commands supported by the FEL protocol. */
enum sunxi_efex_cmd_t {
	EFEX_CMD_VERIFY_DEVICE = 0x0001,
	EFEX_CMD_SWITCH_ROLE = 0x0002,
	EFEX_CMD_IS_READY = 0x0003,
	EFEX_CMD_GET_CMD_SET_VER = 0x0004,
	EFEX_CMD_DISCONNECT = 0x0010,
	EFEX_CMD_FEL_WRITE = 0x0101,
	EFEX_CMD_FEL_EXEC = 0x0102,
	EFEX_CMD_FEL_READ = 0x0103,
};

/** @brief Device mode reported by a FEL verification response. */
enum sunxi_verify_device_mode_t {
	DEVICE_MODE_FEL = 0x1,
};

/** @brief FEL transport request header shared with libefex. */
struct sunxi_usb_request_t {
	union {
		char magic[4];
		uint32_t magics;
	};
	uint32_t tab;
	uint32_t data_length;
	uint16_t resvered1;
	uint8_t resvered2;
	uint8_t cmd_length;
	uint8_t cmd_package[16];
} __attribute__((packed));

/** @brief FEL transport response header shared with libefex. */
struct sunxi_usb_response_t {
	union {
		char magic[4];
		uint32_t magics;
	};
	uint32_t tag;
	uint32_t residue;
	uint8_t status;
} __attribute__((packed));

/** @brief FEL command request sent after transport framing. */
struct sunxi_efex_request_t {
	uint16_t cmd;
	uint16_t tag;
	uint32_t address;
	uint32_t len;
	uint32_t flags;
} __attribute__((packed));

/** @brief FEL command response returned after a request. */
struct sunxi_efex_response_t {
	uint16_t magic;
	uint16_t tag;
	uint8_t status;
	uint8_t reserve[3];
} __attribute__((packed));

/** @brief Device information returned by the FEL verification command. */
struct sunxi_efex_device_resp_t {
	char magic[8];
	uint32_t id;
	uint32_t firmware;
	uint16_t mode;
	uint8_t data_flag;
	uint8_t data_length;
	uint32_t data_start_address;
	uint8_t reserved[8];
};

#endif /* __DRIVERS_USB_FUNCTION_USB_FEL_H__ */
