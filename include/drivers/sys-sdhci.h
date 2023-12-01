/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SDHCI_H__
#define __SDHCI_H__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "sys-gpio.h"

#include "log.h"

typedef enum
{
	MMC_CLK_400K = 0,
	MMC_CLK_25M,
	MMC_CLK_50M,
	MMC_CLK_50M_DDR,
	MMC_CLK_100M,
	MMC_CLK_150M,
	MMC_CLK_200M
} smhc_clk_t;

typedef struct
{
	volatile uint32_t gctrl;		   /* (0x00) SMC Global Control Register */
	volatile uint32_t clkcr;		   /* (0x04) SMC Clock Control Register */
	volatile uint32_t timeout;	   /* (0x08) SMC Time Out Register */
	volatile uint32_t width;		   /* (0x0C) SMC Bus Width Register */
	volatile uint32_t blksz;		   /* (0x10) SMC Block Size Register */
	volatile uint32_t bytecnt;	   /* (0x14) SMC Byte Count Register */
	volatile uint32_t cmd;		   /* (0x18) SMC Command Register */
	volatile uint32_t arg;		   /* (0x1C) SMC Argument Register */
	volatile uint32_t resp0;		   /* (0x20) SMC Response Register 0 */
	volatile uint32_t resp1;		   /* (0x24) SMC Response Register 1 */
	volatile uint32_t resp2;		   /* (0x28) SMC Response Register 2 */
	volatile uint32_t resp3;		   /* (0x2C) SMC Response Register 3 */
	volatile uint32_t imask;		   /* (0x30) SMC Interrupt Mask Register */
	volatile uint32_t mint;		   /* (0x34) SMC Masked Interrupt Status Register */
	volatile uint32_t rint;		   /* (0x38) SMC Raw Interrupt Status Register */
	volatile uint32_t status;	   /* (0x3C) SMC Status Register */
	volatile uint32_t ftrglevel;	   /* (0x40) SMC FIFO Threshold Watermark Register */
	volatile uint32_t funcsel;	   /* (0x44) SMC Function Select Register */
	volatile uint32_t cbcr;		   /* (0x48) SMC CIU Byte Count Register */
	volatile uint32_t bbcr;		   /* (0x4C) SMC BIU Byte Count Register */
	volatile uint32_t dbgc;		   /* (0x50) SMC Debug Enable Register */
	volatile uint32_t csdc;		   /* (0x54) CRC status detect control register*/
	volatile uint32_t a12a;		   /* (0x58)Auto command 12 argument*/
	volatile uint32_t ntsr;		   /* (0x5c)SMC2 Newtiming Set Register */
	volatile uint32_t res1[6];	   /* (0x60~0x74) */
	volatile uint32_t hwrst;		   /* (0x78) SMC eMMC Hardware Reset Register */
	volatile uint32_t res2;		   /*  (0x7c) */
	volatile uint32_t dmac;		   /*  (0x80) SMC IDMAC Control Register */
	volatile uint32_t dlba;		   /*  (0x84) SMC IDMAC Descriptor List Base Address Register */
	volatile uint32_t idst;		   /*  (0x88) SMC IDMAC Status Register */
	volatile uint32_t idie;		   /*  (0x8C) SMC IDMAC Interrupt Enable Register */
	volatile uint32_t chda;		   /*  (0x90) */
	volatile uint32_t cbda;		   /*  (0x94) */
	volatile uint32_t res3[26];	   /*  (0x98~0xff) */
	volatile uint32_t thldc;		   /*  (0x100) Card Threshold Control Register */
	volatile uint32_t sfc;		   /* (0x104) Sample Fifo Control Register */
	volatile uint32_t res4[1];	   /*  (0x10b) */
	volatile uint32_t dsbd;		   /* (0x10c) eMMC4.5 DDR Start Bit Detection Control */
	volatile uint32_t res5[12];	   /* (0x110~0x13c) */
	volatile uint32_t drv_dl;	   /* (0x140) Drive Delay Control register*/
	volatile uint32_t samp_dl;	   /* (0x144) Sample Delay Control register*/
	volatile uint32_t ds_dl;		   /* (0x148) Data Strobe Delay Control Register */
	volatile uint32_t ntdc;		   /* (0x14C) HS400 New Timing Delay Control Register */
	volatile uint32_t res6[4];	   /* (0x150~0x15f) */
	volatile uint32_t skew_dat0_dl; /*(0x160) deskew data0 delay control register*/
	volatile uint32_t skew_dat1_dl; /*(0x164) deskew data1 delay control register*/
	volatile uint32_t skew_dat2_dl; /*(0x168) deskew data2 delay control register*/
	volatile uint32_t skew_dat3_dl; /*(0x16c) deskew data3 delay control register*/
	volatile uint32_t skew_dat4_dl; /*(0x170) deskew data4 delay control register*/
	volatile uint32_t skew_dat5_dl; /*(0x174) deskew data5 delay control register*/
	volatile uint32_t skew_dat6_dl; /*(0x178) deskew data6 delay control register*/
	volatile uint32_t skew_dat7_dl; /*(0x17c) deskew data7 delay control register*/
	volatile uint32_t skew_ds_dl;   /*(0x180) deskew ds delay control register*/
	volatile uint32_t skew_ctrl;	   /*(0x184) deskew control control register*/
	volatile uint32_t res8[30];	   /* (0x188~0x1ff) */
	volatile uint32_t fifo;		   /* (0x200) SMC FIFO Access Address */
	volatile uint32_t res7[63];	   /* (0x201~0x2FF)*/
	volatile uint32_t vers;		   /* (0x300) SMHC Version Register */
} sdhci_reg_t;

typedef struct
{
	uint32_t idx;
	uint32_t arg;
	uint32_t resptype;
	uint32_t response[4];
} sdhci_cmd_t;

typedef struct
{
	uint8_t *buf;
	uint32_t flag;
	uint32_t blksz;
	uint32_t blkcnt;
} sdhci_data_t;

#define SMHC_DES_NUM_SHIFT 12 /* smhc2!! */
#define SMHC_DES_BUFFER_MAX_LEN (1 << SMHC_DES_NUM_SHIFT)
typedef struct
{
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

typedef struct
{
	char *name;
	sdhci_reg_t *reg;
	uint32_t reset;

	uint32_t voltage;
	uint32_t width;
	smhc_clk_t clock;
	uint32_t pclk;
	uint8_t odly[6];
	uint8_t sdly[6];

	sdhci_idma_desc_t dma_desc[32];
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
