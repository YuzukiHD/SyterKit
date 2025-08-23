/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SDHCI_SDHCI_H__
#define __SDHCI_SDHCI_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <reg/reg-smhc.h>
#include <sys-gpio.h>

#include <log.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef enum { MMC_CLK_400K = 0,
			   MMC_CLK_25M,
			   MMC_CLK_50M,
			   MMC_CLK_50M_DDR,
			   MMC_CLK_100M,
			   MMC_CLK_150M,
			   MMC_CLK_200M } smhc_clk_t;

typedef struct {
	uint32_t idx;
	uint32_t arg;
	uint32_t resptype;
	uint32_t response[4];
} sdhci_cmd_t;

typedef struct {
	uint8_t *buf;
	uint32_t flag;
	uint32_t blksz;
	uint32_t blkcnt;
} sdhci_data_t;

#define SMHC_DES_NUM_SHIFT 12 /* smhc2!! */
#define SMHC_DES_BUFFER_MAX_LEN (1 << SMHC_DES_NUM_SHIFT)
typedef struct {
	uint32_t : 1, dic : 1,	/* disable interrupt on completion */
			last_desc : 1,	/* 1-this data buffer is the last buffer */
			first_desc : 1, /* 1-data buffer is the first buffer, 0-data buffer contained in the next descriptor is 1st
						  buffer */
			des_chain : 1,	/* 1-the 2nd address in the descriptor is the next descriptor address */
			// end_of_ring : 1, /* 1-last descriptor flag when using dual data buffer in descriptor */
			: 25, err_flag : 1, /* transfer error flag */
			own : 1;			/* des owner:1-idma owns it, 0-host owns it */

	uint32_t data_buf_sz : SMHC_DES_NUM_SHIFT, data_buf_dummy : (32 - SMHC_DES_NUM_SHIFT);

	uint32_t buf_addr;
	uint32_t next_desc_addr;

} sdhci_idma_desc_t __attribute__((aligned(8)));

typedef enum {
	SDHCI_TYPE_SD = 1,
	SDHCI_TYPE_MMC,
} sdhci_type_t;

typedef struct {
	char *name;
	uint32_t id;
	sdhci_reg_t *reg;
	uint32_t reset;

	uint32_t voltage;
	uint32_t width;
	smhc_clk_t clock;
	uint32_t pclk;
	volatile uint8_t odly[6];
	volatile uint8_t sdly[6];
	volatile sdhci_idma_desc_t dma_desc[32];
	volatile uint32_t sdhci_pll;
	uint32_t dma_trglvl;

	bool removable;
	bool isspi;
	sdhci_type_t sdio_type;
	bool skew_auto_mode;

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
} sdhci_t;

extern sdhci_t sdhci0;

/**
 * @brief Reset the SD card controller.
 * 
 * @param hci A pointer to the SD card controller structure.
 * @return True if the reset is successful, false otherwise.
 */
bool sdhci_reset(sdhci_t *hci);

/**
 * @brief Set the voltage for the SD card controller.
 * 
 * @param hci A pointer to the SD card controller structure.
 * @param voltage The voltage value to be set.
 * @return True if the voltage setting is successful, false otherwise.
 */
bool sdhci_set_voltage(sdhci_t *hci, uint32_t voltage);

/**
 * @brief Set the data bus width for the SD card controller.
 * 
 * @param hci A pointer to the SD card controller structure.
 * @param width The data bus width value to be set.
 * @return True if the data bus width setting is successful, false otherwise.
 */
bool sdhci_set_width(sdhci_t *hci, uint32_t width);

/**
 * @brief Set the clock frequency for the SD card controller.
 * 
 * @param hci A pointer to the SD card controller structure.
 * @param hz The clock frequency value to be set.
 * @return True if the clock frequency setting is successful, false otherwise.
 */
bool sdhci_set_clock(sdhci_t *hci, smhc_clk_t hz);

/**
 * @brief Perform a data transfer operation with the SD card controller.
 * 
 * @param hci A pointer to the SD card controller structure.
 * @param cmd Pointer to the SD command structure.
 * @param dat Pointer to the SD data structure.
 * @return True if the transfer is successful, false otherwise.
 */
bool sdhci_transfer(sdhci_t *hci, sdhci_cmd_t *cmd, sdhci_data_t *dat);

/**
 * @brief Initialize the SD card controller for Sunxi platform.
 * 
 * @param sdhci A pointer to the SD card controller structure.
 * @return 0 if initialization is successful, an error code otherwise.
 */
int sunxi_sdhci_init(sdhci_t *sdhci);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif /* __SDHCI_SDHCI_H__ */
