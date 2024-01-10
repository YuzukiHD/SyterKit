/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SDHCI_H__
#define __SDHCI_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-smhc.h"
#include "sys-gpio.h"

#include "log.h"

typedef enum {
    MMC_CLK_400K = 0,
    MMC_CLK_25M = 1,
    MMC_CLK_50M = 2,
    MMC_CLK_50M_DDR = 3,
    MMC_CLK_100M = 4,
    MMC_CLK_150M = 5,
    MMC_CLK_200M = 6
} smhc_clk_t;

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
    uint32_t : 1, dic : 1,  /* disable interrupt on completion */
            last_desc : 1,  /* 1-this data buffer is the last buffer */
            first_desc : 1, /* 1-data buffer is the first buffer, 0-data buffer contained in the next descriptor is 1st
						  buffer */
            des_chain : 1,  /* 1-the 2nd address in the descriptor is the next descriptor address */
            // end_of_ring : 1, /* 1-last descriptor flag when using dual data buffer in descriptor */
            : 25, err_flag : 1, /* transfer error flag */
            own : 1;            /* des owner:1-idma owns it, 0-host owns it */

    uint32_t data_buf_sz : SMHC_DES_NUM_SHIFT,
                           data_buf_dummy : (32 - SMHC_DES_NUM_SHIFT);

    uint32_t buf_addr;
    uint32_t next_desc_addr;

} sdhci_idma_desc_t __attribute__((aligned(8)));

typedef struct {
    char *name;
    sdhci_reg_t *reg;
    uint32_t reset;

    uint32_t voltage;
    uint32_t width;
    smhc_clk_t clock;
    uint32_t pclk;
    volatile uint8_t odly[6];
    volatile uint8_t sdly[6];

    volatile sdhci_idma_desc_t dma_desc[32];
    uint32_t dma_trglvl;

    bool removable;
    bool isspi;

    gpio_mux_t gpio_d0;
    gpio_mux_t gpio_d1;
    gpio_mux_t gpio_d2;
    gpio_mux_t gpio_d3;
    gpio_mux_t gpio_cmd;
    gpio_mux_t gpio_clk;
} sdhci_t;

extern sdhci_t sdhci0;

bool sdhci_reset(sdhci_t *hci);
bool sdhci_set_voltage(sdhci_t *hci, uint32_t voltage);
bool sdhci_set_width(sdhci_t *hci, uint32_t width);
bool sdhci_set_clock(sdhci_t *hci, smhc_clk_t hz);
bool sdhci_transfer(sdhci_t *hci, sdhci_cmd_t *cmd, sdhci_data_t *dat);
int sunxi_sdhci_init(sdhci_t *sdhci);

#endif /* __SDHCI_H__ */
