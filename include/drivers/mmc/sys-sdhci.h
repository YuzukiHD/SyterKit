/* SPDX-License-Identifier:	GPL-2.0+ */
/* MMC driver for General mmc operations 
 * Original https://github.com/smaeul/sun20i_d1_spl/blob/mainline/drivers/mmc/sun20iw1p1/
 */

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
#define SMHC_WAITBUSY_TIMEOUT 0x4ffffff
#define SMHC_DATA_TIMEOUT 0xffffff
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
    uint32_t : 1, dic : 1,   /* disable interrupt on completion */
            last_desc : 1,   /* 1-this data buffer is the last buffer */
            first_desc : 1,  /* 1-data buffer is the first buffer, 0-data buffer contained in the next descriptor is 1st buffer */
            des_chain : 1,   /* 1-the 2nd address in the descriptor is the next descriptor address */
            end_of_ring : 1, /* 1-last descriptor flag when using dual data buffer in descriptor */
            : 24,            /* Reserved */
            err_flag : 1,    /* transfer error flag */
            own : 1;         /* des owner:1-idma owns it, 0-host owns it */

    uint32_t data_buf_sz : 16, data_buf_dummy : 16;
    uint32_t buf_addr;
    uint32_t next_desc_addr;
} sunxi_sdhci_desc_t __attribute__((aligned(8)));

typedef struct sunxi_sdhci_host {
    sdhci_reg_t *reg;
    uint32_t mclk;
    uint32_t hclkrst;
    uint32_t hclkbase;
    uint32_t mclkbase;
    uint32_t database;
    uint32_t commreg;
    uint32_t fatal_err;
    sunxi_sdhci_desc_t sdhci_desc[32];
    uint32_t timing_mode;
    mmc_t *mmc;
    uint32_t mod_clk;
    uint32_t clock;
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
} sunxi_sdhci_pinctrl_t;

typedef struct sunxi_sdhci_timing {
    uint32_t odly;
    uint32_t sdly;
    uint32_t spd_md_id;
    uint32_t freq_id;
} sunxi_sdhci_timing_t;

typedef struct sdhci {
    char *name;
    uint32_t reg_base;
    uint32_t id;
    uint32_t width;
    uint32_t clk_ctrl_base;
    uint32_t clk_base;
    uint32_t max_clk;
    sunxi_sdhci_type_t type;
    sunxi_sdhci_host_t *mmc_host;
    sunxi_sdhci_pinctrl_t *pinctrl;
    sunxi_sdhci_timing_t *timing_data;
} sdhci_t;

/**
 * @brief Initialize the SDHC controller.
 * 
 * This function initializes the given SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller to initialize.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_init(sdhci_t *sdhci);

/**
 * @brief Initialize the core functionality of the SDHC.
 * 
 * This function initializes the core functionality of the given SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller to initialize.
 */
int sunxi_sdhci_core_init(sdhci_t *sdhci);

/**
 * @brief Set the I/O parameters of the SDHC controller.
 * 
 * This function sets the I/O parameters of the given SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller to set I/O parameters for.
 */
void sunxi_sdhci_set_ios(sdhci_t *sdhci);

/**
 * @brief Perform data transfer operation for the SDHC controller.
 * 
 * This function performs the data transfer operation for the given SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller to perform data transfer operation.
 * @param cmd Pointer to the MMC command structure.
 * @param data Pointer to the MMC data structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_xfer(sdhci_t *sdhci, mmc_cmd_t *cmd, mmc_data_t *data);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_SDHCI_H_