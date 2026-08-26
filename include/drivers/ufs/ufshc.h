/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFSHC_H__
#define __SYTERKIT_UFSHC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <log.h>

#ifdef CONFIG_DRIVER_UFS_DEBUG
#define ufs_debug(fmt, ...) printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define ufs_debug(fmt, ...) no_printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#endif

#define UFSHC_ERR_INVALID (-1)
#define UFSHC_ERR_IO	  (-2)
#define UFSHC_ERR_TIMEOUT (-3)

/* UFSHCI register offsets.  Keeping these in the host-controller layer makes
 * the SCSI and device layers independent of a particular UFSHCI revision. */
#define UFSHC_REG_CAP		       0x00U
#define UFSHC_REG_VERSION	       0x08U
#define UFSHC_REG_INTERRUPT_STATUS     0x20U
#define UFSHC_REG_INTERRUPT_ENABLE     0x24U
#define UFSHC_REG_CONTROLLER_STATUS    0x30U
#define UFSHC_REG_CONTROLLER_ENABLE    0x34U
#define UFSHC_REG_UIC_ERROR_PHY_ADAPTER 0x38U
#define UFSHC_REG_UIC_ERROR_DATA_LINK   0x3cU
#define UFSHC_REG_UIC_ERROR_NETWORK     0x40U
#define UFSHC_REG_UIC_ERROR_TRANSPORT   0x44U
#define UFSHC_REG_UIC_ERROR_DME         0x48U
#define UFSHC_REG_UTRL_INT_AGG_CONTROL 0x4cU
#define UFSHC_REG_UTRL_BASE_L	       0x50U
#define UFSHC_REG_UTRL_BASE_H	       0x54U
#define UFSHC_REG_UTRL_DOOR_BELL       0x58U
#define UFSHC_REG_UTRL_LIST_CLEAR       0x5cU
#define UFSHC_REG_UTRL_RUN_STOP	       0x60U
#define UFSHC_REG_UTMRL_BASE_L	       0x70U
#define UFSHC_REG_UTMRL_BASE_H	       0x74U
#define UFSHC_REG_UTMRL_DOOR_BELL       0x78U
#define UFSHC_REG_UTMRL_LIST_CLEAR       0x7cU
#define UFSHC_REG_UTMRL_RUN_STOP       0x80U
#define UFSHC_REG_UIC_COMMAND	       0x90U
#define UFSHC_REG_UIC_ARG1	       0x94U
#define UFSHC_REG_UIC_ARG2	       0x98U
#define UFSHC_REG_UIC_ARG3	       0x9cU

/* Controller status power-mode change request status (UPMCRS). */
#define UFSHC_HCS_UPMCRS_SHIFT 8U
#define UFSHC_HCS_UPMCRS_MASK  (0x7U << UFSHC_HCS_UPMCRS_SHIFT)
#define UFSHC_PWR_OK	       0U
#define UFSHC_PWR_LOCAL	       1U

#define UFSHC_HCE		 (1U << 0)
#define UFSHC_HCS_READY		 0x0eU
#define UFSHC_HCS_DEVICE_PRESENT (1U << 0)

/* Interrupt status bits used by the polling implementation. */
#define UFSHC_INT_TRANSFER_COMPLETE (1U << 0)
#define UFSHC_INT_TASK_COMPLETE	    (1U << 9)
#define UFSHC_INT_UIC_POWER_MODE    (1U << 4)
#define UFSHC_INT_UIC_LINK_STARTUP  (1U << 8)
#define UFSHC_INT_UIC_COMPLETE	    (1U << 10)
/* UCCS is the ordinary completion indication for the command issued through
 * UIC_CMD.  Power-mode control also reports completion through UPMS; callers
 * clear pending status before issuing a command and wait for either event. */
#define UFSHC_INT_UIC_COMPLETE_MASK UFSHC_INT_UIC_COMPLETE
#define UFSHC_INT_ERROR		    ((1U << 2) | (1U << 11) | (1U << 16) | (1U << 17))

#define UFSHC_UIC_DME_GET      0x01U
#define UFSHC_UIC_DME_SET      0x02U
#define UFSHC_UIC_DME_PEER_GET 0x03U
#define UFSHC_UIC_DME_PEER_SET 0x04U
#define UFSHC_UIC_LINK_STARTUP 0x16U

#define UFSHC_UIC_RESULT_MASK	0xffU
#define UFSHC_TIMEOUT_US	1000000U
#define UFSHC_DEV_MANAGEMENT_TIMEOUT_US 3000000U
#define UFSHC_PHY_INIT_TIMEOUT_US 10000U
#define UFSHC_LINK_STARTUP_RETRIES 3U
#define UFSHC_MAX_PRDT		32U
#define UFSHC_PRDT_MAX_BYTES	(256U * 1024U)
#define UFSHC_CDB_SIZE		16U
#define UFSHC_UPIU_SIZE		512U
#define UFSHC_UPIU_HEADER_SIZE	12U
#define UFSHC_UPIU_QUERY_SIZE	20U
#define UFSHC_UPIU_DATA_OFFSET	(UFSHC_UPIU_HEADER_SIZE + UFSHC_UPIU_QUERY_SIZE)
#define UFSHC_SCSI_SENSE_OFFSET 34U
#define UFSHC_SCSI_SENSE_SIZE	18U

/* UPIU transaction types. */
#define UFSHC_UPIU_NOP_OUT   0x00U
#define UFSHC_UPIU_COMMAND   0x01U
#define UFSHC_UPIU_TASK_REQ  0x04U
#define UFSHC_UPIU_NOP_IN    0x20U
#define UFSHC_UPIU_RESPONSE  0x21U
#define UFSHC_UPIU_TASK_RSP  0x24U
#define UFSHC_UPIU_QUERY_REQ 0x16U
#define UFSHC_UPIU_QUERY_RSP 0x36U
#define UFSHC_UPIU_REJECT    0x3fU

/* Query request functions and opcodes. */
#define UFSHC_QUERY_FUNC_STANDARD_READ	0x01U
#define UFSHC_QUERY_FUNC_STANDARD_WRITE 0x81U
#define UFSHC_QUERY_OPCODE_READ_DESC	0x01U
#define UFSHC_QUERY_OPCODE_WRITE_DESC	0x02U
#define UFSHC_QUERY_OPCODE_READ_ATTR	0x03U
#define UFSHC_QUERY_OPCODE_WRITE_ATTR	0x04U
#define UFSHC_QUERY_OPCODE_READ_FLAG	0x05U
#define UFSHC_QUERY_OPCODE_SET_FLAG	0x06U
#define UFSHC_QUERY_OPCODE_CLEAR_FLAG	0x07U
#define UFSHC_QUERY_OPCODE_TOGGLE_FLAG	0x08U

/* Standard UFS query flag identifiers. */
#define UFSHC_QUERY_FLAG_FDEVICE_INIT 0x01U
#define UFSHC_QUERY_ATTR_REF_CLK_FREQ 0x0aU

/* UFS task-management function codes. */
#define UFSHC_TASK_ABORT 0x01U
#define UFSHC_TASK_ABORT_SET 0x02U
#define UFSHC_TASK_CLEAR_SET 0x04U
#define UFSHC_TASK_LOGICAL_RESET 0x08U
#define UFSHC_TASK_QUERY 0x80U
#define UFSHC_TASK_QUERY_SET 0x81U

/* UniPro PA-layer attributes used during the initial power-mode change. */
#define UFSHC_PA_ACTIVETXDATALANES    0x1560U
#define UFSHC_PA_CONNECTEDTXDATALANES 0x1561U
#define UFSHC_PA_TXGEAR		      0x1568U
#define UFSHC_PA_TXTERMINATION	      0x1569U
#define UFSHC_PA_HSSERIES	      0x156aU
#define UFSHC_PA_PWRMODE	      0x1571U
#define UFSHC_PA_ACTIVERXDATALANES    0x1580U
#define UFSHC_PA_CONNECTEDRXDATALANES 0x1581U
#define UFSHC_PA_RXGEAR		      0x1583U
#define UFSHC_PA_RXTERMINATION	      0x1584U
#define UFSHC_PA_MAXRXPWMGEAR	      0x1586U
#define UFSHC_PA_MAXRXHSGEAR	      0x1587U
#define UFSHC_PA_TXHSADAPTTYPE	      0x15d4U
#define UFSHC_PA_PWRMODEUSERDATA0      0x15b0U
#define UFSHC_PA_PWRMODEUSERDATA1      0x15b1U
#define UFSHC_PA_PWRMODEUSERDATA2      0x15b2U
#define UFSHC_PA_PWRMODEUSERDATA3      0x15b3U
#define UFSHC_PA_PWRMODEUSERDATA4      0x15b4U
#define UFSHC_PA_PWRMODEUSERDATA5      0x15b5U
#define UFSHC_DME_LOCAL_FC0_TIMEOUT    0xd041U
#define UFSHC_DME_LOCAL_TC0_TIMEOUT    0xd042U
#define UFSHC_DME_LOCAL_AFC0_TIMEOUT   0xd043U
#define UFSHC_DL_FC0_TIMEOUT_DEFAULT   8191U
#define UFSHC_DL_TC0_TIMEOUT_DEFAULT   65535U
#define UFSHC_DL_AFC0_TIMEOUT_DEFAULT  32767U
#define UFSHC_PA_INITIAL_ADAPT_GEAR    4U

#define UFSHC_PWR_FAST	       1U
#define UFSHC_PWR_SLOW	       2U
#define UFSHC_HS_RATE_A	       1U
#define UFSHC_HS_RATE_B	       2U
#define UFSHC_PA_INITIAL_ADAPT 1U
#define UFSHC_PA_NO_ADAPT      3U

/* UTP request descriptor flags. */
#define UFSHC_REQ_INT		    0x01000000U
#define UFSHC_REQ_HOST_TO_DEVICE    0x02000000U
#define UFSHC_REQ_DEVICE_TO_HOST    0x04000000U
#define UFSHC_REQ_CMD_TYPE_SCSI	    0x00000000U
#define UFSHC_REQ_CMD_TYPE_UFS_STORAGE 0x10000000U
#define UFSHC_REQ_CMD_TYPE_DEV_MGMT 0x20000000U
#define UFSHC_OCS_MASK		    0x0fU

struct ufshc_host;

struct ufshc_platform_ops {
	/* These callbacks own SoC-specific clocks, reset and PHY sequencing. */
	int (*enable)(void *priv);
	void (*disable)(void *priv);
	int (*phy_init)(void *priv);
	/* Return the encoded bRefClkFreq value used by the active PHY clock. */
	int (*get_ref_clk_freq)(void *priv, uint32_t *value);
	/* Return the calibrated PA_HSSERIES value (A or B). */
	int (*get_hs_rate)(void *priv, uint32_t *value);
	/* Controller-specific setup happens after clocks are on and before HCE. */
	int (*prepare)(uintptr_t base, void *priv);
	/* Assert and release the device-side RST_n input before HCE. */
	void (*device_reset)(uintptr_t base, void *priv);
	/* PHY/UniPro hooks run around the standard DME LINK STARTUP command. */
	int (*link_startup)(struct ufshc_host *host, void *priv);
	int (*link_up)(struct ufshc_host *host, void *priv);
	void *priv;
};

struct ufshc_power_mode {
	uint8_t pwr_tx;
	uint8_t pwr_rx;
	uint8_t gear_tx;
	uint8_t gear_rx;
	uint8_t lane_tx;
	uint8_t lane_rx;
	uint8_t hs_rate;
};

struct ufshc_config {
	uintptr_t base;
	uint32_t timeout_us;
	const struct ufshc_platform_ops *platform;
	void *platform_priv;
};

struct ufshc_request {
	uint8_t lun;
	uint8_t cdb[UFSHC_CDB_SIZE];
	uint8_t cdb_len;
	void *data;
	size_t data_len;
	bool write;
	uint8_t response_type;
	/* UPIU response code (kept under the historical task_response name). */
	uint8_t task_response;
	uint8_t status;
	uint8_t sense_length;
	uint8_t sense[UFSHC_SCSI_SENSE_SIZE];
	uint32_t residual_transfer_count;
};

struct ufshc_prd {
	uint32_t base;
	uint32_t upper;
	uint32_t reserved;
	uint32_t size;
};

struct ufshc_request_desc {
	uint32_t header[4];
	uint32_t command_base_lo;
	uint32_t command_base_hi;
	uint16_t response_length;
	uint16_t response_offset;
	uint16_t prdt_length;
	uint16_t prdt_offset;
} __attribute__((packed));

/* One task-management slot is enough for early boot while retaining the
 * standard 20-dword UTMRD request/response layout. */
struct ufshc_task_request_desc {
	uint32_t header[4];
	uint32_t request_header[3];
	uint32_t input_param[3];
	uint32_t request_reserved[2];
	uint32_t response_header[3];
	uint32_t output_param[2];
	uint32_t response_reserved[3];
} __attribute__((packed));

struct ufshc_command_desc {
	uint8_t command_upiu[UFSHC_UPIU_SIZE];
	uint8_t response_upiu[UFSHC_UPIU_SIZE];
	struct ufshc_prd prdt[UFSHC_MAX_PRDT];
};

struct ufshc_host {
	uintptr_t base;
	uint32_t version;
	uint32_t capabilities;
	uint32_t timeout_us;
	bool initialized;
	bool platform_active;
	bool controller_enabled;
	struct ufshc_platform_ops platform_ops;
	const struct ufshc_platform_ops *platform;
};

struct ufshc_uic_cmd_args {
	uint32_t command;
	uint32_t argument1;
	uint32_t argument2;
	uint32_t argument3;
};

int ufshc_init(struct ufshc_host *host, const struct ufshc_config *config);
void ufshc_exit(struct ufshc_host *host);
int ufshc_exec(struct ufshc_host *host, struct ufshc_request *request);
int ufshc_uic_command(struct ufshc_host *host, const struct ufshc_uic_cmd_args *args, uint32_t *result);
int ufshc_dme_get(struct ufshc_host *host, uint32_t attribute, uint32_t *value, bool peer);
int ufshc_dme_set(struct ufshc_host *host, uint32_t attribute, uint32_t value, bool peer);
int ufshc_dme_get_sel(struct ufshc_host *host, uint32_t attribute, uint16_t selector, uint32_t *value, bool peer);
int ufshc_dme_set_sel(struct ufshc_host *host, uint32_t attribute, uint16_t selector, uint32_t value, bool peer);
int ufshc_get_max_power_mode(struct ufshc_host *host, struct ufshc_power_mode *mode);
int ufshc_change_power_mode(struct ufshc_host *host, const struct ufshc_power_mode *mode);
int ufshc_nop(struct ufshc_host *host);
int ufshc_query_flag(struct ufshc_host *host, uint8_t idn, bool set, bool *value);
int ufshc_query_flag_op(struct ufshc_host *host, uint8_t idn, uint8_t opcode,
			bool *value);
int ufshc_query_attribute(
	struct ufshc_host *host, uint8_t idn, uint8_t index, uint8_t selector, uint32_t *value, bool write);
int ufshc_query_descriptor(struct ufshc_host *host, uint8_t idn, uint8_t index, uint8_t selector, void *buffer,
	size_t buffer_len, size_t *actual_len);
int ufshc_query_descriptor_op(struct ufshc_host *host, uint8_t opcode,
			       uint8_t idn, uint8_t index, uint8_t selector,
			       void *buffer, size_t buffer_len, size_t *actual_len);
int ufshc_task_request(struct ufshc_host *host, uint8_t lun,
			       uint8_t function, uint16_t task_id,
			       uint8_t *service_response);

#endif /* __SYTERKIT_UFSHC_H__ */
