/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef _SYS_SDHCI_H_
#define _SYS_SDHCI_H_

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <reg/reg-smhc.h>
#include <sys-gpio.h>

#include <log.h>

#include "sys-mmc.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define SMHC_DES_NUM_SHIFT 12 /* smhc2!! */
#define SMHC_DES_BUFFER_MAX_LEN (1 << SMHC_DES_NUM_SHIFT)

#define MMC_REG_FIFO_OS (0x200)

#define SMHC_TIMEOUT 0xfffff
#define SMHC_DMA_TIMEOUT 0xffffff
#define SMHC_WAITBUSY_TIMEOUT 0xfffff
#define SMHC_DATA_TIMEOUT 0xfffff
#define SMHC_RESP_TIMEOUT 0xff

enum {
	MMC_CONTROLLER_0 = 0,
	MMC_CONTROLLER_1 = 1,
	MMC_CONTROLLER_2 = 2,
};

typedef enum {
	MMC_TYPE_SD,
	MMC_TYPE_EMMC,
} sunxi_sdhci_type_t;

typedef struct sunxi_sdhci_desc {
	uint32_t : 1, dic : 1,	 /* disable interrupt on completion */
			last_desc : 1,	 /* 1-this data buffer is the last buffer */
			first_desc : 1,	 /* 1-data buffer is the first buffer, 0-data buffer contained in the next descriptor is 1st buffer */
			des_chain : 1,	 /* 1-the 2nd address in the descriptor is the next descriptor address */
			end_of_ring : 1, /* 1-last descriptor flag when using dual data buffer in descriptor */
			: 24,			 /* Reserved */
			err_flag : 1,	 /* transfer error flag */
			own : 1;		 /* des owner:1-idma owns it, 0-host owns it */

	uint32_t data_buf_sz : 16, data_buf_dummy : 16;
	uint32_t buf_addr;
	uint32_t next_desc_addr;
} sunxi_sdhci_desc_t __attribute__((aligned(8)));

typedef struct sunxi_sdhci_host {
	sdhci_reg_t *reg;
	uint32_t commreg;
	uint8_t fatal_err;
	uint8_t timing_mode;

	/* DMA DESC */
	sunxi_sdhci_desc_t *sdhci_desc;
} sunxi_sdhci_host_t;

typedef struct sunxi_sdhci_pinctrl {
	gpio_mux_t gpio_d0;
	gpio_mux_t gpio_d1;
	gpio_mux_t gpio_d2;
	gpio_mux_t gpio_d3;
	gpio_mux_t gpio_d4;
	gpio_mux_t gpio_d5;
	gpio_mux_t gpio_d6;
	gpio_mux_t gpio_d7;
	gpio_mux_t gpio_cmd;
	gpio_mux_t gpio_clk;
	gpio_mux_t gpio_ds;
	gpio_mux_t gpio_rst;
	gpio_mux_t gpio_cd;
	uint8_t cd_level;
} sunxi_sdhci_pinctrl_t;

typedef struct sunxi_sdhci_timing {
	uint32_t odly;
	uint32_t sdly;
	uint8_t auto_timing;
} sunxi_sdhci_timing_t;

typedef struct sunxi_sdhci_clk {
	uint32_t reg_base;
	uint8_t factor_n;
	uint8_t reg_factor_n_offset;
	uint8_t factor_m;
	uint8_t reg_factor_m_offset;
	uint8_t clk_sel;
	uint32_t parent_clk;
} sunxi_sdhci_clk_t;

typedef struct sunxi_sdhci {
	char *name;
	uint32_t reg_base;
	uint32_t id;
	uint32_t width;
	sunxi_clk_t clk_ctrl;
	sunxi_sdhci_clk_t sdhci_clk;
	uint32_t max_clk;
	uint32_t dma_des_addr;
	sunxi_sdhci_type_t sdhci_mmc_type;

	/* Pinctrl info */
	sunxi_sdhci_pinctrl_t pinctrl;

	/* Private data */
	mmc_t *mmc;
	sunxi_sdhci_host_t *mmc_host;
	sunxi_sdhci_timing_t *timing_data;
} sunxi_sdhci_t;

#define SDHCI_DEFAULT_CLK_RST_OFFSET(x) (16 + x)
#define SDHCI_DEFAULT_CLK_GATE_OFFSET(x) (x)
#define SDHCI_DEFAULT_CLK_FACTOR_M_OFFSET (0)
#define SDHCI_DEFAULT_CLK_FACTOR_N_OFFSET (8)

/**
 * @brief Initialize the SDHC controller.
 * 
 * This function initializes the SDHC controller by configuring its parameters,
 * capabilities, and features based on the provided SDHC structure. It sets up
 * the controller's timing mode, supported voltages, host capabilities, clock
 * frequency limits, and register addresses. Additionally, it configures pin
 * settings and enables clocks for the SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_init(sunxi_sdhci_t *sdhci);

/**
 * @brief Initialize the core functionality of the SDHC controller.
 * 
 * This function initializes the core functionality of the SDHC controller,
 * including resetting the controller, setting timeout values, configuring
 * thresholds and debug parameters, and releasing the eMMC reset signal.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_core_init(sunxi_sdhci_t *sdhci);

/**
 * @brief Set the I/O settings for the SDHC controller.
 * 
 * This function configures the I/O settings for the SDHC controller based on the
 * provided MMC clock, bus width, and speed mode.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return void
 */
void sunxi_sdhci_set_ios(sunxi_sdhci_t *sdhci);

/**
 * @brief Update phase for the SDHC controller.
 * 
 * This function updates the phase for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Returns 0 on success.
 */
int sunxi_sdhci_update_phase(sunxi_sdhci_t *sdhci);

/**
 * @brief Perform a data transfer operation on the SDHC controller.
 * 
 * This function performs a data transfer operation on the SDHC controller,
 * including sending a command and managing data transfer if present. It also
 * handles error conditions such as fatal errors and card busy status.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param cmd Pointer to the MMC command structure.
 * @param data Pointer to the MMC data structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_xfer(sunxi_sdhci_t *sdhci, mmc_cmd_t *cmd, mmc_data_t *data);

/**
 * @brief Dump the contents of the SDHCI registers.
 *
 * This function dumps the contents of the SDHCI registers for a given SD card host controller.
 *
 * @param sdhci A pointer to the structure representing the SD card host controller.
 * @return void
 *
 * @note This function is useful for debugging and analyzing the state of the SD card controller.
 */
void sunxi_sdhci_dump_reg(sunxi_sdhci_t *sdhci);

/* Internal Use */
/**
 * @brief Set the SDHC controller's clock frequency.
 * 
 * This function sets the clock frequency for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param clk_hz Desired clock frequency in Hertz.
 * @return Returns 0 on success, -1 on failure.
 */
extern int sunxi_sdhci_set_mclk(sunxi_sdhci_t *sdhci, uint32_t clk_hz);

/**
 * @brief Get the current clock frequency of the SDHC controller.
 * 
 * This function retrieves the current clock frequency of the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Current clock frequency in Hertz.
 */
extern uint32_t sunxi_sdhci_get_mclk(sunxi_sdhci_t *sdhci);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_SDHCI_H_