/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __UFS_H__
#define __UFS_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-ufs.h"

#include "blk.h"
#include "scsi.h"

#define UFS_CDB_SIZE 16
#define RESPONSE_UPIU_SENSE_DATA_LENGTH 18
#define UFS_MAX_LUNS 0x7F
#define UFSHCD_QUIRK_BROKEN_LCC 0x1

/* Controller UFSHCI version */
enum {
	UFSHCI_VERSION_10 = 0x00010000, /* 1.0 */
	UFSHCI_VERSION_11 = 0x00010100, /* 1.1 */
	UFSHCI_VERSION_20 = 0x00000200, /* 2.0 */
	UFSHCI_VERSION_21 = 0x00000210, /* 2.1 */
};

/* Interrupt disable masks */
enum {
	/* Interrupt disable mask for UFSHCI v1.0 */
	INTERRUPT_MASK_ALL_VER_10 = 0x30FFF,
	INTERRUPT_MASK_RW_VER_10 = 0x30000,
	/* Interrupt disable mask for UFSHCI v1.1 */
	INTERRUPT_MASK_ALL_VER_11 = 0x31FFF,
	/* Interrupt disable mask for UFSHCI v2.1 */
	INTERRUPT_MASK_ALL_VER_21 = 0x71FFF,
};

enum {
	PWR_OK = 0x0,
	PWR_LOCAL = 0x01,
	PWR_REMOTE = 0x02,
	PWR_BUSY = 0x03,
	PWR_ERROR_CAP = 0x04,
	PWR_FATAL_ERROR = 0x05,
};

enum {
	TASK_REQ_UPIU_SIZE_DWORDS = 8,
	TASK_RSP_UPIU_SIZE_DWORDS = 8,
	ALIGNED_UPIU_SIZE = 512,
};

enum {
	UTP_CMD_TYPE_SCSI = 0x0,
	UTP_CMD_TYPE_UFS = 0x1,
	UTP_CMD_TYPE_DEV_MANAGE = 0x2,
};

/* UTP Transfer Request Command Offset */
#define UPIU_COMMAND_TYPE_OFFSET 28

/* Offset of the response code in the UPIU header */
#define UPIU_RSP_CODE_OFFSET 8

/* To accommodate UFS2.0 required Command type */
enum {
	UTP_CMD_TYPE_UFS_STORAGE = 0x1,
};

enum {
	UTP_SCSI_COMMAND = 0x00000000,
	UTP_NATIVE_UFS_COMMAND = 0x10000000,
	UTP_DEVICE_MANAGEMENT_FUNCTION = 0x20000000,
	UTP_REQ_DESC_INT_CMD = 0x01000000,
};

/* UTP Transfer Request Data Direction (DD) */
enum {
	UTP_NO_DATA_TRANSFER = 0x00000000,
	UTP_HOST_TO_DEVICE = 0x02000000,
	UTP_DEVICE_TO_HOST = 0x04000000,
};

/* Overall command status values */
enum {
	OCS_SUCCESS = 0x0,
	OCS_INVALID_CMD_TABLE_ATTR = 0x1,
	OCS_INVALID_PRDT_ATTR = 0x2,
	OCS_MISMATCH_DATA_BUF_SIZE = 0x3,
	OCS_MISMATCH_RESP_UPIU_SIZE = 0x4,
	OCS_PEER_COMM_FAILURE = 0x5,
	OCS_ABORTED = 0x6,
	OCS_FATAL_ERROR = 0x7,
	OCS_INVALID_COMMAND_STATUS = 0x0F,
	MASK_OCS = 0x0F,
};

/*
 * UFS Protocol Information Unit related definitions
 */
/* Task management functions */
enum {
	UFS_ABORT_TASK = 0x01,
	UFS_ABORT_TASK_SET = 0x02,
	UFS_CLEAR_TASK_SET = 0x04,
	UFS_LOGICAL_RESET = 0x08,
	UFS_QUERY_TASK = 0x80,
	UFS_QUERY_TASK_SET = 0x81,
};

/* UTP UPIU Transaction Codes Initiator to Target */
enum {
	UPIU_TRANSACTION_NOP_OUT = 0x00,
	UPIU_TRANSACTION_COMMAND = 0x01,
	UPIU_TRANSACTION_DATA_OUT = 0x02,
	UPIU_TRANSACTION_TASK_REQ = 0x04,
	UPIU_TRANSACTION_QUERY_REQ = 0x16,
};

/* UTP UPIU Transaction Codes Target to Initiator */
enum {
	UPIU_TRANSACTION_NOP_IN = 0x20,
	UPIU_TRANSACTION_RESPONSE = 0x21,
	UPIU_TRANSACTION_DATA_IN = 0x22,
	UPIU_TRANSACTION_TASK_RSP = 0x24,
	UPIU_TRANSACTION_READY_XFER = 0x31,
	UPIU_TRANSACTION_QUERY_RSP = 0x36,
	UPIU_TRANSACTION_REJECT_UPIU = 0x3F,
};

/* UPIU Read/Write flags */
enum {
	UPIU_CMD_FLAGS_NONE = 0x00,
	UPIU_CMD_FLAGS_WRITE = 0x20,
	UPIU_CMD_FLAGS_READ = 0x40,
};

/* UPIU Task Attributes */
enum {
	UPIU_TASK_ATTR_SIMPLE = 0x00,
	UPIU_TASK_ATTR_ORDERED = 0x01,
	UPIU_TASK_ATTR_HEADQ = 0x02,
	UPIU_TASK_ATTR_ACA = 0x03,
};

/* UPIU Query request function */
enum {
	UPIU_QUERY_FUNC_STANDARD_READ_REQUEST = 0x01,
	UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST = 0x81,
};

/* Offset of the response code in the UPIU header */
#define UPIU_RSP_CODE_OFFSET 8

enum {
	MASK_SCSI_STATUS = 0xFF,
	MASK_TASK_RESPONSE = 0xFF00,
	MASK_RSP_UPIU_RESULT = 0xFFFF,
	MASK_QUERY_DATA_SEG_LEN = 0xFFFF,
	MASK_RSP_UPIU_DATA_SEG_LEN = 0xFFFF,
	MASK_RSP_EXCEPTION_EVENT = 0x10000,
	MASK_TM_SERVICE_RESP = 0xFF,
	MASK_TM_FUNC = 0xFF,
};

/* UTP QUERY Transaction Specific Fields OpCode */
enum query_opcode {
	UPIU_QUERY_OPCODE_NOP = 0x0,
	UPIU_QUERY_OPCODE_READ_DESC = 0x1,
	UPIU_QUERY_OPCODE_WRITE_DESC = 0x2,
	UPIU_QUERY_OPCODE_READ_ATTR = 0x3,
	UPIU_QUERY_OPCODE_WRITE_ATTR = 0x4,
	UPIU_QUERY_OPCODE_READ_FLAG = 0x5,
	UPIU_QUERY_OPCODE_SET_FLAG = 0x6,
	UPIU_QUERY_OPCODE_CLEAR_FLAG = 0x7,
	UPIU_QUERY_OPCODE_TOGGLE_FLAG = 0x8,
};

/* bRefClkFreq attribute values */
enum ufs_ref_clk_freq {
	REF_CLK_FREQ_19_2_MHZ = 0,
	REF_CLK_FREQ_26_MHZ = 1,
	REF_CLK_FREQ_38_4_MHZ = 2,
	REF_CLK_FREQ_52_MHZ = 3,
	REF_CLK_FREQ_INVAL = -1,
};

/* Query response result code */
enum {
	QUERY_RESULT_SUCCESS = 0x00,
	QUERY_RESULT_NOT_READABLE = 0xF6,
	QUERY_RESULT_NOT_WRITEABLE = 0xF7,
	QUERY_RESULT_ALREADY_WRITTEN = 0xF8,
	QUERY_RESULT_INVALID_LENGTH = 0xF9,
	QUERY_RESULT_INVALID_VALUE = 0xFA,
	QUERY_RESULT_INVALID_SELECTOR = 0xFB,
	QUERY_RESULT_INVALID_INDEX = 0xFC,
	QUERY_RESULT_INVALID_IDN = 0xFD,
	QUERY_RESULT_INVALID_OPCODE = 0xFE,
	QUERY_RESULT_GENERAL_FAILURE = 0xFF,
};

enum ufs_dev_pwr_mode {
	UFS_ACTIVE_PWR_MODE = 1,
	UFS_SLEEP_PWR_MODE = 2,
	UFS_POWERDOWN_PWR_MODE = 3,
};

enum ufs_notify_change_status {
	PRE_CHANGE,
	POST_CHANGE,
};

enum {
	UPIU_COMMAND_SET_TYPE_SCSI = 0x0,
	UPIU_COMMAND_SET_TYPE_UFS = 0x1,
	UPIU_COMMAND_SET_TYPE_QUERY = 0x2,
};

/* Flag idn for Query Requests*/
enum flag_idn {
	QUERY_FLAG_IDN_FDEVICEINIT = 0x01,
	QUERY_FLAG_IDN_PERMANENT_WPE = 0x02,
	QUERY_FLAG_IDN_PWR_ON_WPE = 0x03,
	QUERY_FLAG_IDN_BKOPS_EN = 0x04,
	QUERY_FLAG_IDN_LIFE_SPAN_MODE_ENABLE = 0x05,
	QUERY_FLAG_IDN_PURGE_ENABLE = 0x06,
	QUERY_FLAG_IDN_RESERVED2 = 0x07,
	QUERY_FLAG_IDN_FPHYRESOURCEREMOVAL = 0x08,
	QUERY_FLAG_IDN_BUSY_RTC = 0x09,
	QUERY_FLAG_IDN_RESERVED3 = 0x0A,
	QUERY_FLAG_IDN_PERMANENTLY_DISABLE_FW_UPDATE = 0x0B,
};

/* Attribute idn for Query requests */
enum attr_idn {
	QUERY_ATTR_IDN_BOOT_LU_EN = 0x00,
	QUERY_ATTR_IDN_RESERVED = 0x01,
	QUERY_ATTR_IDN_POWER_MODE = 0x02,
	QUERY_ATTR_IDN_ACTIVE_ICC_LVL = 0x03,
	QUERY_ATTR_IDN_OOO_DATA_EN = 0x04,
	QUERY_ATTR_IDN_BKOPS_STATUS = 0x05,
	QUERY_ATTR_IDN_PURGE_STATUS = 0x06,
	QUERY_ATTR_IDN_MAX_DATA_IN = 0x07,
	QUERY_ATTR_IDN_MAX_DATA_OUT = 0x08,
	QUERY_ATTR_IDN_DYN_CAP_NEEDED = 0x09,
	QUERY_ATTR_IDN_REF_CLK_FREQ = 0x0A,
	QUERY_ATTR_IDN_CONF_DESC_LOCK = 0x0B,
	QUERY_ATTR_IDN_MAX_NUM_OF_RTT = 0x0C,
	QUERY_ATTR_IDN_EE_CONTROL = 0x0D,
	QUERY_ATTR_IDN_EE_STATUS = 0x0E,
	QUERY_ATTR_IDN_SECONDS_PASSED = 0x0F,
	QUERY_ATTR_IDN_CNTX_CONF = 0x10,
	QUERY_ATTR_IDN_CORR_PRG_BLK_NUM = 0x11,
	QUERY_ATTR_IDN_RESERVED2 = 0x12,
	QUERY_ATTR_IDN_RESERVED3 = 0x13,
	QUERY_ATTR_IDN_FFU_STATUS = 0x14,
	QUERY_ATTR_IDN_PSA_STATE = 0x15,
	QUERY_ATTR_IDN_PSA_DATA_SIZE = 0x16,
};

/* Descriptor idn for Query requests */
enum desc_idn {
	QUERY_DESC_IDN_DEVICE = 0x0,
	QUERY_DESC_IDN_CONFIGURATION = 0x1,
	QUERY_DESC_IDN_UNIT = 0x2,
	QUERY_DESC_IDN_RFU_0 = 0x3,
	QUERY_DESC_IDN_INTERCONNECT = 0x4,
	QUERY_DESC_IDN_STRING = 0x5,
	QUERY_DESC_IDN_RFU_1 = 0x6,
	QUERY_DESC_IDN_GEOMETRY = 0x7,
	QUERY_DESC_IDN_POWER = 0x8,
	QUERY_DESC_IDN_HEALTH = 0x9,
	QUERY_DESC_IDN_MAX,
};

enum desc_header_offset {
	QUERY_DESC_LENGTH_OFFSET = 0x00,
	QUERY_DESC_DESC_TYPE_OFFSET = 0x01,
};

enum ufs_desc_def_size {
	QUERY_DESC_DEVICE_DEF_SIZE = 0x40,
	QUERY_DESC_CONFIGURATION_DEF_SIZE = 0x90,
	QUERY_DESC_UNIT_DEF_SIZE = 0x23,
	QUERY_DESC_INTERCONNECT_DEF_SIZE = 0x06,
	QUERY_DESC_GEOMETRY_DEF_SIZE = 0x48,
	QUERY_DESC_POWER_DEF_SIZE = 0x62,
	QUERY_DESC_HEALTH_DEF_SIZE = 0x25,
};

/* Device descriptor parameters offsets in bytes*/
enum device_desc_param {
	DEVICE_DESC_PARAM_LEN = 0x0,
	DEVICE_DESC_PARAM_TYPE = 0x1,
	DEVICE_DESC_PARAM_DEVICE_TYPE = 0x2,
	DEVICE_DESC_PARAM_DEVICE_CLASS = 0x3,
	DEVICE_DESC_PARAM_DEVICE_SUB_CLASS = 0x4,
	DEVICE_DESC_PARAM_PRTCL = 0x5,
	DEVICE_DESC_PARAM_NUM_LU = 0x6,
	DEVICE_DESC_PARAM_NUM_WLU = 0x7,
	DEVICE_DESC_PARAM_BOOT_ENBL = 0x8,
	DEVICE_DESC_PARAM_DESC_ACCSS_ENBL = 0x9,
	DEVICE_DESC_PARAM_INIT_PWR_MODE = 0xA,
	DEVICE_DESC_PARAM_HIGH_PR_LUN = 0xB,
	DEVICE_DESC_PARAM_SEC_RMV_TYPE = 0xC,
	DEVICE_DESC_PARAM_SEC_LU = 0xD,
	DEVICE_DESC_PARAM_BKOP_TERM_LT = 0xE,
	DEVICE_DESC_PARAM_ACTVE_ICC_LVL = 0xF,
	DEVICE_DESC_PARAM_SPEC_VER = 0x10,
	DEVICE_DESC_PARAM_MANF_DATE = 0x12,
	DEVICE_DESC_PARAM_MANF_NAME = 0x14,
	DEVICE_DESC_PARAM_PRDCT_NAME = 0x15,
	DEVICE_DESC_PARAM_SN = 0x16,
	DEVICE_DESC_PARAM_OEM_ID = 0x17,
	DEVICE_DESC_PARAM_MANF_ID = 0x18,
	DEVICE_DESC_PARAM_UD_OFFSET = 0x1A,
	DEVICE_DESC_PARAM_UD_LEN = 0x1B,
	DEVICE_DESC_PARAM_RTT_CAP = 0x1C,
	DEVICE_DESC_PARAM_FRQ_RTC = 0x1D,
	DEVICE_DESC_PARAM_UFS_FEAT = 0x1F,
	DEVICE_DESC_PARAM_FFU_TMT = 0x20,
	DEVICE_DESC_PARAM_Q_DPTH = 0x21,
	DEVICE_DESC_PARAM_DEV_VER = 0x22,
	DEVICE_DESC_PARAM_NUM_SEC_WPA = 0x24,
	DEVICE_DESC_PARAM_PSA_MAX_DATA = 0x25,
	DEVICE_DESC_PARAM_PSA_TMT = 0x29,
	DEVICE_DESC_PARAM_PRDCT_REV = 0x2A,
};

enum {
	UFSHCD_MAX_CHANNEL = 0,
	UFSHCD_MAX_ID = 1,
};

enum dev_cmd_type {
	DEV_CMD_TYPE_NOP = 0x0,
	DEV_CMD_TYPE_QUERY = 0x1,
};

/* Link Status*/
enum link_status {
	UFSHCD_LINK_IS_DOWN = 1,
	UFSHCD_LINK_IS_UP = 2,
};

/* UIC Commands */
enum uic_cmd_dme {
	UIC_CMD_DME_GET = 0x01,
	UIC_CMD_DME_SET = 0x02,
	UIC_CMD_DME_PEER_GET = 0x03,
	UIC_CMD_DME_PEER_SET = 0x04,
	UIC_CMD_DME_POWERON = 0x10,
	UIC_CMD_DME_POWEROFF = 0x11,
	UIC_CMD_DME_ENABLE = 0x12,
	UIC_CMD_DME_RESET = 0x14,
	UIC_CMD_DME_END_PT_RST = 0x15,
	UIC_CMD_DME_LINK_STARTUP = 0x16,
	UIC_CMD_DME_HIBER_ENTER = 0x17,
	UIC_CMD_DME_HIBER_EXIT = 0x18,
	UIC_CMD_DME_TEST_MODE = 0x1A,
};

/* UIC Config result code / Generic error code */
enum {
	UIC_CMD_RESULT_SUCCESS = 0x00,
	UIC_CMD_RESULT_INVALID_ATTR = 0x01,
	UIC_CMD_RESULT_FAILURE = 0x01,
	UIC_CMD_RESULT_INVALID_ATTR_VALUE = 0x02,
	UIC_CMD_RESULT_READ_ONLY_ATTR = 0x03,
	UIC_CMD_RESULT_WRITE_ONLY_ATTR = 0x04,
	UIC_CMD_RESULT_BAD_INDEX = 0x05,
	UIC_CMD_RESULT_LOCKED_ATTR = 0x06,
	UIC_CMD_RESULT_BAD_TEST_FEATURE_INDEX = 0x07,
	UIC_CMD_RESULT_PEER_COMM_FAILURE = 0x08,
	UIC_CMD_RESULT_BUSY = 0x09,
	UIC_CMD_RESULT_DME_FAILURE = 0x0A,
};

#define MASK_UIC_COMMAND_RESULT 0xFF

typedef struct ufs_pa_layer_attr {
	uint32_t gear_rx;
	uint32_t gear_tx;
	uint32_t lane_rx;
	uint32_t lane_tx;
	uint32_t pwr_rx;
	uint32_t pwr_tx;
	uint32_t hs_rate;
} ufs_pa_layer_attr_t;

typedef struct ufs_pwr_mode_info {
	bool is_valid;
	ufs_pa_layer_attr_t info;
} ufs_pwr_mode_info_t;

typedef struct ufshcd_sg_entry {
	uint32_t base_addr;
	uint32_t upper_addr;
	uint32_t reserved;
	uint32_t size;
} ufshcd_sg_entry_t;

#define MAX_BUFF (16 * 4)//4M*4
typedef struct utp_transfer_cmd_desc {
	uint8_t command_upiu[ALIGNED_UPIU_SIZE];
	uint8_t response_upiu[ALIGNED_UPIU_SIZE];
	ufshcd_sg_entry_t prd_table[MAX_BUFF];
} utp_transfer_cmd_desc_t;

/**
 * struct request_desc_header - Descriptor Header common to both UTRD and UTMRD
 * @dword0: Descriptor Header DW0
 * @dword1: Descriptor Header DW1
 * @dword2: Descriptor Header DW2
 * @dword3: Descriptor Header DW3
 */
typedef struct request_desc_header {
	uint32_t dword_0;
	uint32_t dword_1;
	uint32_t dword_2;
	uint32_t dword_3;
} request_desc_header_t;

/**
 * struct utp_transfer_req_desc - UTRD structure
 * @header: UTRD header DW-0 to DW-3
 * @command_desc_base_addr_lo: UCD base address low DW-4
 * @command_desc_base_addr_hi: UCD base address high DW-5
 * @response_upiu_length: response UPIU length DW-6
 * @response_upiu_offset: response UPIU offset DW-6
 * @prd_table_length: Physical region descriptor length DW-7
 * @prd_table_offset: Physical region descriptor offset DW-7
 */
typedef struct utp_transfer_req_desc {
	/* DW 0-3 */
	request_desc_header_t header;

	/* DW 4-5*/
	uint32_t command_desc_base_addr_lo;
	uint32_t command_desc_base_addr_hi;

	/* DW 6 */
	uint16_t response_upiu_length;
	uint16_t response_upiu_offset;

	/* DW 7 */
	uint16_t prd_table_length;
	uint16_t prd_table_offset;
} utp_transfer_req_desc_t;

/**
 * struct utp_upiu_header - UPIU header structure
 * @dword_0: UPIU header DW-0
 * @dword_1: UPIU header DW-1
 * @dword_2: UPIU header DW-2
 */
typedef struct utp_upiu_header {
	uint32_t dword_0;
	uint32_t dword_1;
	uint32_t dword_2;
} utp_upiu_header_t;

/**
 * struct utp_upiu_query - upiu request buffer structure for
 * query request.
 * @opcode: command to perform B-0
 * @idn: a value that indicates the particular type of data B-1
 * @index: Index to further identify data B-2
 * @selector: Index to further identify data B-3
 * @reserved_osf: spec reserved field B-4,5
 * @length: number of descriptor bytes to read/write B-6,7
 * @value: Attribute value to be written DW-5
 * @reserved: spec reserved DW-6,7
 */
typedef struct utp_upiu_query {
	uint8_t opcode;
	uint8_t idn;
	uint8_t index;
	uint8_t selector;
	uint16_t reserved_osf;
	uint16_t length;
	uint32_t value;
	uint32_t reserved[2];
} utp_upiu_query_t;

/**
 * struct utp_upiu_cmd - Command UPIU structure
 * @data_transfer_len: Data Transfer Length DW-3
 * @cdb: Command Descriptor Block CDB DW-4 to DW-7
 */
typedef struct utp_upiu_cmd {
	uint32_t exp_data_transfer_len;
	uint8_t cdb[UFS_CDB_SIZE];
} utp_upiu_cmd_t;

/*
 * UTMRD structure.
 */
typedef struct utp_task_req_desc {
	/* DW 0-3 */
	request_desc_header_t header;

	/* DW 4-11 - Task request UPIU structure */
	utp_upiu_header_t req_header;
	uint32_t input_param1;
	uint32_t input_param2;
	uint32_t input_param3;
	uint32_t __reserved1[2];

	/* DW 12-19 - Task Management Response UPIU structure */
	utp_upiu_header_t rsp_header;
	uint32_t output_param1;
	uint32_t output_param2;
	uint32_t __reserved2[3];
} utp_task_req_desc_t;

/**
 * struct utp_upiu_req - general upiu request structure
 * @header:UPIU header structure DW-0 to DW-2
 * @sc: fields structure for scsi command DW-3 to DW-7
 * @qr: fields structure for query request DW-3 to DW-7
 */
//#pragma anon_unions
typedef struct utp_upiu_req {
	utp_upiu_header_t header;
	union {
		utp_upiu_cmd_t sc;
		utp_upiu_query_t qr;
		utp_upiu_query_t tr;
		/* use utp_upiu_query to host the 4 dwords of uic command */
		utp_upiu_query_t uc;
	};
} utp_upiu_req_t;

/**
 * struct utp_cmd_rsp - Response UPIU structure
 * @residual_transfer_count: Residual transfer count DW-3
 * @reserved: Reserved double words DW-4 to DW-7
 * @sense_data_len: Sense data length DW-8 U16
 * @sense_data: Sense data field DW-8 to DW-12
 */
typedef struct utp_cmd_rsp {
	uint32_t residual_transfer_count;
	uint32_t reserved[4];
	uint16_t sense_data_len;
	uint8_t sense_data[RESPONSE_UPIU_SENSE_DATA_LENGTH];
} utp_cmd_rsp_t;

/**
 * struct utp_upiu_rsp - general upiu response structure
 * @header: UPIU header structure DW-0 to DW-2
 * @sr: fields structure for scsi command DW-3 to DW-12
 * @qr: fields structure for query request DW-3 to DW-7
 */
typedef struct utp_upiu_rsp {
	utp_upiu_header_t header;
	union {
		utp_cmd_rsp_t sr;
		utp_upiu_query_t qr;
	};
} utp_upiu_rsp_t;

#define MAX_MODEL_LEN 16
/**
 * ufs_dev_desc - ufs device details from the device descriptor
 *
 * @wmanufacturerid: card details
 * @model: card model
 */
typedef struct ufs_dev_desc {
	uint16_t wmanufacturerid;
	char model[MAX_MODEL_LEN + 1];
} ufs_dev_desc_t;

/**
 * struct uic_command - UIC command structure
 * @command: UIC command
 * @argument1: UIC command argument 1
 * @argument2: UIC command argument 2
 * @argument3: UIC command argument 3
 * @cmd_active: Indicate if UIC command is outstanding
 * @result: UIC command result
 * @done: UIC command completion
 */
typedef struct uic_command {
	uint32_t command;
	uint32_t argument1;
	uint32_t argument2;
	uint32_t argument3;
	int cmd_active;
	int result;
} uic_command_t;


#define GENERAL_UPIU_REQUEST_SIZE (sizeof(utp_upiu_req_t))
#define QUERY_DESC_MAX_SIZE 255
#define QUERY_DESC_MIN_SIZE 2
#define QUERY_DESC_HDR_SIZE 2
#define QUERY_OSF_SIZE (GENERAL_UPIU_REQUEST_SIZE - (sizeof(utp_upiu_header_t)))
#define RESPONSE_UPIU_SENSE_DATA_LENGTH 18
#define UPIU_HEADER_DWORD(byte3, byte2, byte1, byte0) cpu_to_be32((byte3 << 24) | (byte2 << 16) | (byte1 << 8) | (byte0))
/* GenSelectorIndex calculation macros for M-PHY attributes */
#define UIC_ARG_MPHY_TX_GEN_SEL_INDEX(lane) (lane)
#define UIC_ARG_MPHY_RX_GEN_SEL_INDEX(lane) (PA_MAXDATALANES + (lane))

#define UIC_ARG_MIB_SEL(attr, sel) ((((((uint32_t) attr)) & 0xFFFF) << 16) | ((sel) &0xFFFF))
#define UIC_ARG_MIB(attr) UIC_ARG_MIB_SEL(((uint32_t) attr), 0)
#define UIC_ARG_ATTR_TYPE(t) (((t) &0xFF) << 16)
#define UIC_GET_ATTR_ID(v) (((v) >> 16) & 0xFFFF)

/* Host <-> Device UniPro Link state */
enum uic_link_state {
	UIC_LINK_OFF_STATE = 0,		/* Link powered down or disabled */
	UIC_LINK_ACTIVE_STATE = 1,	/* Link is in Fast/Slow/Sleep state */
	UIC_LINK_HIBERN8_STATE = 2, /* Link is in Hibernate state */
};

/* UIC command interfaces for DME primitives */
#define DME_LOCAL 0
#define DME_PEER 1
#define ATTR_SET_NOR 0 /* NORMAL */
#define ATTR_SET_ST 1  /* STATIC */

/**
 * struct ufs_query_req - parameters for building a query request
 * @query_func: UPIU header query function
 * @upiu_req: the query request data
 */
typedef struct ufs_query_req {
	uint8_t query_func;
	utp_upiu_query_t upiu_req;
} ufs_query_req_t;

/**
 * struct ufs_query_resp - UPIU QUERY
 * @response: device response code
 * @upiu_res: query response data
 */
typedef struct ufs_query_res {
	uint8_t response;
	utp_upiu_query_t upiu_res;
} ufs_query_res_t;

/**
 * struct ufs_query - holds relevant data structures for query request
 * @request: request upiu and function
 * @descriptor: buffer for sending/receiving descriptor
 * @response: response upiu and response
 */
typedef struct ufs_query {
	ufs_query_req_t request;
	uint8_t *descriptor;
	ufs_query_res_t response;
} ufs_query_t;

/**
 * struct ufs_dev_cmd - all assosiated fields with device management commands
 * @type: device management command type - Query, NOP OUT
 * @tag_wq: wait queue until free command slot is available
 */
typedef struct ufs_dev_cmd {
	enum dev_cmd_type type;
	ufs_query_t query;
} ufs_dev_cmd_t;

typedef struct ufs_desc_size {
	uint32_t dev_desc;
	uint32_t pwr_desc;
	uint32_t geom_desc;
	uint32_t interc_desc;
	uint32_t unit_desc;
	uint32_t conf_desc;
	uint32_t hlth_desc;
} ufs_desc_size_t;

typedef struct ufs_basic_info {
	uint32_t capabilities;
	uint32_t version;
	uint32_t interrupt_mask;
	uint32_t quirks;
} ufs_basic_info_t;

typedef struct ufs_hba {
	ufs_basic_info_t basic_info;
	ufs_desc_size_t desc_size;

	/* Virtual memory reference */
	utp_transfer_cmd_desc_t *ucdl;
	utp_transfer_req_desc_t *utrdl;

	/* Task Manegement Support */
	utp_task_req_desc_t *utmrdl;

	utp_upiu_req_t *ucd_req_ptr;
	utp_upiu_rsp_t *ucd_rsp_ptr;
	ufshcd_sg_entry_t *ucd_prdt_ptr;

	/* Power Mode information */
	uint32_t curr_dev_pwr_mode;
	ufs_pa_layer_attr_t pwr_info;
	ufs_pwr_mode_info_t max_pwr_info;

	ufs_dev_cmd_t dev_cmd;
	uint32_t dev_ref_clk_freq;
} ufs_hba_t;

typedef struct ufs_device {
	ufs_hba_t ufs_hba;
	scsi_plat_t sc_plat;
	void *bd;
} ufs_device_t;

#endif// __UFS_H__