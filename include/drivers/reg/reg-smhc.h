/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_SMHC_H__
#define __REG_SMHC_H__

/*
 * Global control register bits
 */
#define SMHC_GCTRL_SOFT_RESET (1 << 0)
#define SMHC_GCTRL_FIFO_RESET (1 << 1)
#define SMHC_GCTRL_DMA_RESET (1 << 2)
#define SMHC_GCTRL_INTERRUPT_ENABLE (1 << 4)
#define SMHC_GCTRL_DMA_ENABLE (1 << 5)
#define SMHC_GCTRL_DEBOUNCE_ENABLE (1 << 8)
#define SMHC_GCTRL_POSEDGE_LATCH_DATA (1 << 9)
#define SMHC_GCTRL_DDR_MODE (1 << 10)
#define SMHC_GCTRL_MEMORY_ACCESS_DONE (1 << 29)
#define SMHC_GCTRL_ACCESS_DONE_DIRECT (1 << 30)
#define SMHC_GCTRL_ACCESS_BY_AHB (1 << 31)
#define SMHC_GCTRL_ACCESS_BY_DMA (0 << 31)
#define SMHC_GCTRL_HARDWARE_RESET (SMHC_GCTRL_SOFT_RESET | SMHC_GCTRL_FIFO_RESET | SMHC_GCTRL_DMA_RESET)

/*
 * Clock control bits
 */
#define SMHC_CLKCR_MASK_D0 (1 << 31)
#define SMHC_CLKCR_CARD_CLOCK_ON (1 << 16)
#define SMHC_CLKCR_LOW_POWER_ON (1 << 17)
#define SMHC_CLKCR_CLOCK_DIV(n) ((n - 1) & 0xff)

/*
 * Bus width
 */
#define SMHC_WIDTH_1BIT (0)
#define SMHC_WIDTH_4BIT (1)
#define SMHC_WIDTH_8BIT (2)

/*
 * Smc command bits
 */
#define SMHC_CMD_RESP_EXPIRE (1 << 6)
#define SMHC_CMD_LONG_RESPONSE (1 << 7)
#define SMHC_CMD_CHECK_RESPONSE_CRC (1 << 8)
#define SMHC_CMD_DATA_EXPIRE (1 << 9)
#define SMHC_CMD_WRITE (1 << 10)
#define SMHC_CMD_SEQUENCE_MODE (1 << 11)
#define SMHC_CMD_SEND_AUTO_STOP (1 << 12)
#define SMHC_CMD_WAIT_PRE_OVER (1 << 13)
#define SMHC_CMD_STOP_ABORT_CMD (1 << 14)
#define SMHC_CMD_SEND_INIT_SEQUENCE (1 << 15)
#define SMHC_CMD_UPCLK_ONLY (1 << 21)
#define SMHC_CMD_READ_CEATA_DEV (1 << 22)
#define SMHC_CMD_CCS_EXPIRE (1 << 23)
#define SMHC_CMD_ENABLE_BIT_BOOT (1 << 24)
#define SMHC_CMD_ALT_BOOT_OPTIONS (1 << 25)
#define SMHC_CMD_BOOT_ACK_EXPIRE (1 << 26)
#define SMHC_CMD_BOOT_ABORT (1 << 27)
#define SMHC_CMD_VOLTAGE_SWITCH (1 << 28)
#define SMHC_CMD_USE_HOLD_REGISTER (1 << 29)
#define SMHC_CMD_START (1 << 31)

/*
 * Interrupt bits
 */
#define SMHC_RINT_RESP_ERROR (0x1 << 1)
#define SMHC_RINT_COMMAND_DONE (0x1 << 2)
#define SMHC_RINT_DATA_OVER (0x1 << 3)
#define SMHC_RINT_TX_DATA_REQUEST (0x1 << 4)
#define SMHC_RINT_RX_DATA_REQUEST (0x1 << 5)
#define SMHC_RINT_RESP_CRC_ERROR (0x1 << 6)
#define SMHC_RINT_DATA_CRC_ERROR (0x1 << 7)
#define SMHC_RINT_RESP_TIMEOUT (0x1 << 8)
#define SMHC_RINT_DATA_TIMEOUT (0x1 << 9)
#define SMHC_RINT_VOLTAGE_CHANGE_DONE (0x1 << 10)
#define SMHC_RINT_FIFO_RUN_ERROR (0x1 << 11)
#define SMHC_RINT_HARD_WARE_LOCKED (0x1 << 12)
#define SMHC_RINT_START_BIT_ERROR (0x1 << 13)
#define SMHC_RINT_AUTO_COMMAND_DONE (0x1 << 14)
#define SMHC_RINT_END_BIT_ERROR (0x1 << 15)
#define SMHC_RINT_SDIO_INTERRUPT (0x1 << 16)
#define SMHC_RINT_R1B_BUSY_CLEAR (0x1 << 17)
#define SMHC_RINT_CARD_INSERT (0x1 << 30)
#define SMHC_RINT_CARD_REMOVE (0x1 << 31)
#define SMHC_RINT_INTERRUPT_ERROR_BIT                                                                                                                          \
	(SMHC_RINT_RESP_ERROR | SMHC_RINT_RESP_CRC_ERROR | SMHC_RINT_DATA_CRC_ERROR | SMHC_RINT_RESP_TIMEOUT | SMHC_RINT_DATA_TIMEOUT | SMHC_RINT_FIFO_RUN_ERROR | \
	 SMHC_RINT_HARD_WARE_LOCKED | SMHC_RINT_START_BIT_ERROR | SMHC_RINT_END_BIT_ERROR) /* 0xbbc2 */
#define SMHC_RINT_INTERRUPT_DONE_BIT (SMHC_RINT_AUTO_COMMAND_DONE | SMHC_RINT_DATA_OVER | SMHC_RINT_COMMAND_DONE | SMHC_RINT_VOLTAGE_CHANGE_DONE)

/*
 * Status
 */
#define SMHC_STATUS_RXWL_FLAG (1 << 0)
#define SMHC_STATUS_TXWL_FLAG (1 << 1)
#define SMHC_STATUS_FIFO_EMPTY (1 << 2)
#define SMHC_STATUS_FIFO_FULL (1 << 3)
#define SMHC_STATUS_CARD_PRESENT (1 << 8)
#define SMHC_STATUS_CARD_DATA_BUSY (1 << 9)
#define SMHC_STATUS_DATA_FSM_BUSY (1 << 10)
#define SMHC_STATUS_DMA_REQUEST (1 << 31)
#define SMHC_STATUS_FIFO_SIZE (16)
#define SMHC_STATUS_FIFO_LEVEL(x) (((x) >> 17) & 0x3fff)

/* IDMA controller bus mod bit field */
#define SMHC_IDMAC_SOFT_RESET BIT(0)
#define SMHC_IDMAC_FIX_BURST BIT(1)
#define SMHC_IDMAC_IDMA_ON BIT(7)
#define SMHC_IDMAC_REFETCH_DES BIT(31)

/* IDMA status bit field */
#define SMHC_IDMAC_TRANSMIT_INTERRUPT BIT(0)
#define SMHC_IDMAC_RECEIVE_INTERRUPT BIT(1)
#define SMHC_IDMAC_FATAL_BUS_ERROR BIT(2)
#define SMHC_IDMAC_DESTINATION_INVALID BIT(4)
#define SMHC_IDMAC_CARD_ERROR_SUM BIT(5)
#define SMHC_IDMAC_NORMAL_INTERRUPT_SUM BIT(8)
#define SMHC_IDMAC_ABNORMAL_INTERRUPT_SUM BIT(9)
#define SMHC_IDMAC_HOST_ABORT_INTERRUPT BIT(10)
#define SMHC_IDMAC_IDLE (0 << 13)
#define SMHC_IDMAC_SUSPEND (1 << 13)
#define SMHC_IDMAC_DESC_READ (2 << 13)
#define SMHC_IDMAC_DESC_CHECK (3 << 13)
#define SMHC_IDMAC_READ_REQUEST_WAIT (4 << 13)
#define SMHC_IDMAC_WRITE_REQUEST_WAIT (5 << 13)
#define SMHC_IDMAC_READ (6 << 13)
#define SMHC_IDMAC_WRITE (7 << 13)
#define SMHC_IDMAC_DESC_CLOSE (8 << 13)

/*
 * If the idma-des-size-bits of property is ie 13, bufsize bits are:
 *  Bits  0-12: buf1 size
 *  Bits 13-25: buf2 size
 *  Bits 26-31: not used
 * Since we only ever set buf1 size, we can simply store it directly.
 */
#define SMHC_IDMAC_DES0_DIC BIT(1)	/* disable interrupt on completion */
#define SMHC_IDMAC_DES0_LD BIT(2)	/* last descriptor */
#define SMHC_IDMAC_DES0_FD BIT(3)	/* first descriptor */
#define SMHC_IDMAC_DES0_CH BIT(4)	/* chain mode */
#define SMHC_IDMAC_DES0_ER BIT(5)	/* end of ring */
#define SMHC_IDMAC_DES0_CES BIT(30) /* card error summary */
#define SMHC_IDMAC_DES0_OWN BIT(31) /* 1-idma owns it, 0-host owns it */

/*
timing mode
0: output and input are both based on [0,1,...,7] pll delay.
1: output and input are both based on phase.
2: output is based on phase, input is based on delay chain except hs400.
    input of hs400 is based on delay chain.
3: output is based on phase, input is based on delay chain.
4: output is based on phase, input is based on delay chain.
    it also support to use delay chain on data strobe signal.
*/
#define SUNXI_MMC_TIMING_MODE_0 0U
#define SUNXI_MMC_TIMING_MODE_1 1U
#define SUNXI_MMC_TIMING_MODE_2 2U
#define SUNXI_MMC_TIMING_MODE_3 3U
#define SUNXI_MMC_TIMING_MODE_4 4U
#define SUNXI_MMC_TIMING_MODE_5 5U

#define MMC_CLK_SAMPLE_POINIT_MODE_0 8U
#define MMC_CLK_SAMPLE_POINIT_MODE_1 3U
#define MMC_CLK_SAMPLE_POINIT_MODE_2 2U
#define MMC_CLK_SAMPLE_POINIT_MODE_2_HS400 64U
#define MMC_CLK_SAMPLE_POINIT_MODE_3 64U
#define MMC_CLK_SAMPLE_POINIT_MODE_4 64U
#define MMC_CLK_SAMPLE_POINIT_MODE_5 64U

#define TM5_OUT_PH90 (0)
#define TM5_OUT_PH180 (1)
#define TM5_IN_PH90 (0)
#define TM5_IN_PH180 (1)
#define TM5_IN_PH270 (2)
#define TM5_IN_PH0 (3)

/* delay control */
#define SDXC_NTDC_START_CAL (1 << 15)
#define SDXC_NTDC_CAL_DONE (1 << 14)
#define SDXC_NTDC_CAL_DLY (0x3F << 8)
#define SDXC_NTDC_ENABLE_DLY (1 << 7)
#define SDXC_NTDC_CFG_DLY (0x3F << 0)
#define SDXC_NTDC_CFG_NEW_DLY (0xF << 0)

#define DTO_MAX 200
#define SUNXI_MMC_NTSR_MODE_SEL_NEW (0x1 << 31)

typedef struct {
	volatile uint32_t gctrl;		/* (0x00) SMC Global Control Register */
	volatile uint32_t clkcr;		/* (0x04) SMC Clock Control Register */
	volatile uint32_t timeout;		/* (0x08) SMC Time Out Register */
	volatile uint32_t width;		/* (0x0C) SMC Bus Width Register */
	volatile uint32_t blksz;		/* (0x10) SMC Block Size Register */
	volatile uint32_t bytecnt;		/* (0x14) SMC Byte Count Register */
	volatile uint32_t cmd;			/* (0x18) SMC Command Register */
	volatile uint32_t arg;			/* (0x1C) SMC Argument Register */
	volatile uint32_t resp0;		/* (0x20) SMC Response Register 0 */
	volatile uint32_t resp1;		/* (0x24) SMC Response Register 1 */
	volatile uint32_t resp2;		/* (0x28) SMC Response Register 2 */
	volatile uint32_t resp3;		/* (0x2C) SMC Response Register 3 */
	volatile uint32_t imask;		/* (0x30) SMC Interrupt Mask Register */
	volatile uint32_t mint;			/* (0x34) SMC Masked Interrupt Status Register */
	volatile uint32_t rint;			/* (0x38) SMC Raw Interrupt Status Register */
	volatile uint32_t status;		/* (0x3C) SMC Status Register */
	volatile uint32_t ftrglevel;	/* (0x40) SMC FIFO Threshold Watermark Register */
	volatile uint32_t funcsel;		/* (0x44) SMC Function Select Register */
	volatile uint32_t cbcr;			/* (0x48) SMC CIU Byte Count Register */
	volatile uint32_t bbcr;			/* (0x4C) SMC BIU Byte Count Register */
	volatile uint32_t dbgc;			/* (0x50) SMC Debug Enable Register */
	volatile uint32_t csdc;			/* (0x54) CRC status detect control register*/
	volatile uint32_t a12a;			/* (0x58)Auto command 12 argument*/
	volatile uint32_t ntsr;			/* (0x5c)SMC2 Newtiming Set Register */
	volatile uint32_t res1[6];		/* (0x60~0x74) */
	volatile uint32_t hwrst;		/* (0x78) SMC eMMC Hardware Reset Register */
	volatile uint32_t res2;			/*  (0x7c) */
	volatile uint32_t dmac;			/*  (0x80) SMC IDMAC Control Register */
	volatile uint32_t dlba;			/*  (0x84) SMC IDMAC Descriptor List Base Address Register */
	volatile uint32_t idst;			/*  (0x88) SMC IDMAC Status Register */
	volatile uint32_t idie;			/*  (0x8C) SMC IDMAC Interrupt Enable Register */
	volatile uint32_t chda;			/*  (0x90) */
	volatile uint32_t cbda;			/*  (0x94) */
	volatile uint32_t res3[26];		/*  (0x98~0xff) */
	volatile uint32_t thldc;		/*  (0x100) Card Threshold Control Register */
	volatile uint32_t sfc;			/* (0x104) Sample Fifo Control Register */
	volatile uint32_t res4[1];		/*  (0x10b) */
	volatile uint32_t dsbd;			/* (0x10c) eMMC4.5 DDR Start Bit Detection Control */
	volatile uint32_t res5[12];		/* (0x110~0x13c) */
	volatile uint32_t drv_dl;		/* (0x140) Drive Delay Control register*/
	volatile uint32_t samp_dl;		/* (0x144) Sample Delay Control register*/
	volatile uint32_t ds_dl;		/* (0x148) Data Strobe Delay Control Register */
	volatile uint32_t ntdc;			/* (0x14C) HS400 New Timing Delay Control Register */
	volatile uint32_t res6[4];		/* (0x150~0x15f) */
	volatile uint32_t skew_dat0_dl; /*(0x160) deskew data0 delay control register*/
	volatile uint32_t skew_dat1_dl; /*(0x164) deskew data1 delay control register*/
	volatile uint32_t skew_dat2_dl; /*(0x168) deskew data2 delay control register*/
	volatile uint32_t skew_dat3_dl; /*(0x16c) deskew data3 delay control register*/
	volatile uint32_t skew_dat4_dl; /*(0x170) deskew data4 delay control register*/
	volatile uint32_t skew_dat5_dl; /*(0x174) deskew data5 delay control register*/
	volatile uint32_t skew_dat6_dl; /*(0x178) deskew data6 delay control register*/
	volatile uint32_t skew_dat7_dl; /*(0x17c) deskew data7 delay control register*/
	volatile uint32_t skew_ds_dl;	/*(0x180) deskew ds delay control register*/
	volatile uint32_t skew_ctrl;	/*(0x184) deskew control control register*/
	volatile uint32_t res8[30];		/* (0x188~0x1ff) */
	volatile uint32_t fifo;			/* (0x200) SMC FIFO Access Address */
	volatile uint32_t res7[63];		/* (0x201~0x2FF)*/
	volatile uint32_t vers;			/* (0x300) SMHC Version Register */
} sdhci_reg_t;

#endif// __REG_SMHC_H__