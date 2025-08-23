/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_SPI_H__
#define __REG_SPI_H__

#include <stdint.h>

/* SPI Global Control Register Bit Fields & Masks,default value:0x0000_0080 */
#define SPI_GC_EN (0x1 << 0)	/* SPI module enable control 1:enable; 0:disable; default:0 */
#define SPI_GC_MODE (0x1 << 1)	/* SPI function mode select 1:master; 0:slave; default:0 */
#define SPI_GC_TP_EN (0x1 << 7) /* SPI transmit stop enable 1:stop transmit data when RXFIFO is full; 0:ignore RXFIFO status; default:1 */
#define SPI_GC_SRST (0x1 << 31) /* soft reset, write 1 will clear SPI control, auto clear to 0 */

/* SPI Transfer Control Register Bit Fields & Masks,default value:0x0000_0087 */
#define SPI_TC_PHA (0x1 << 0)	   /* SPI Clock/Data phase control,0: phase0,1: phase1;default:1 */
#define SPI_TC_POL (0x1 << 1)	   /* SPI Clock polarity control,0:low level idle,1:high level idle;default:1 */
#define SPI_TC_SPOL (0x1 << 2)	   /* SPI Chip select signal polarity control,default: 1,low effective like this:~~|_____~~ */
#define SPI_TC_SSCTL (0x1 << 3)	   /* SPI chip select control,default 0:SPI_SSx remains asserted between SPI bursts,1:negate SPI_SSx between SPI bursts */
#define SPI_TC_SS_MASK (0x3 << 4)  /* SPI chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPI_TC_SS_OWNER (0x1 << 6) /* SS output mode select default is 0:automatic output SS;1:manual output SS */
#define SPI_TC_SS_LEVEL (0x1 << 7) /* defautl is 1:set SS to high;0:set SS to low */
#define SPI_TC_DHB (0x1 << 8)	   /* Discard Hash Burst,default 0:receiving all spi burst in BC period 1:discard unused,fectch WTC bursts */
#define SPI_TC_DDB (0x1 << 9)	   /* Dummy burst Type,default 0: dummy spi burst is zero;1:dummy spi burst is one */
#define SPI_TC_RPSM (0x1 << 10)	   /* select mode for high speed write,0:normal write mode,1:rapids write mode,default 0 */
#define SPI_TC_SDC (0x1 << 11)	   /* master sample data control, 1: delay--high speed operation;0:no delay. */
#define SPI_TC_FBS (0x1 << 12)	   /* LSB/MSB transfer first select 0:MSB,1:LSB,default 0:MSB first */
#define SPI_TC_SDM (0x1 << 13)	   /* master sample data mode, SDM = 1:Normal Sample Mode, SDM = 0:Delay Sample Mode */
#define SPI_TC_SDC1 (0x1 << 15)	   /* master sample data mode, SDM = 1:Normal Sample Mode, SDM = 0:Delay Sample Mode */
#define SPI_TC_XCH (0x1 << 31)	   /* Exchange burst default 0:idle,1:start exchange;when BC is zero,this bit cleared by SPI controller*/
#define SPI_TC_SS_BIT_POS (4)

/* SPI Interrupt Control Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_INTEN_RX_RDY (0x1 << 0)											   /* rxFIFO Ready Interrupt Enable,---used for immediately received,0:disable;1:enable */
#define SPI_INTEN_RX_EMP (0x1 << 1)											   /* rxFIFO Empty Interrupt Enable ---used for IRQ received */
#define SPI_INTEN_RX_FULL (0x1 << 2)										   /* rxFIFO Full Interrupt Enable ---seldom used */
#define SPI_INTEN_TX_ERQ (0x1 << 4)											   /* txFIFO Empty Request Interrupt Enable ---seldom used */
#define SPI_INTEN_TX_EMP (0x1 << 5)											   /* txFIFO Empty Interrupt Enable ---used  for IRQ tx */
#define SPI_INTEN_TX_FULL (0x1 << 6)										   /* txFIFO Full Interrupt Enable ---seldom used */
#define SPI_INTEN_RX_OVF (0x1 << 8)											   /* rxFIFO Overflow Interrupt Enable ---used for error detect */
#define SPI_INTEN_RX_UDR (0x1 << 9)											   /* rxFIFO Underrun Interrupt Enable ---used for error detect */
#define SPI_INTEN_TX_OVF (0x1 << 10)										   /* txFIFO Overflow Interrupt Enable ---used for error detect */
#define SPI_INTEN_TX_UDR (0x1 << 11)										   /* txFIFO Underrun Interrupt Enable ---not happened */
#define SPI_INTEN_TC (0x1 << 12)											   /* Transfer Completed Interrupt Enable  ---used */
#define SPI_INTEN_SSI (0x1 << 13)											   /* SSI interrupt Enable,chip select from valid state to invalid state,for slave used only */
#define SPI_INTEN_ERR (SPI_INTEN_TX_OVF | SPI_INTEN_RX_UDR | SPI_INTEN_RX_OVF) /* NO txFIFO underrun */
#define SPI_INTEN_MASK (0x77 | (0x3f << 8))

/* SPI Interrupt Status Register Bit Fields & Masks,default value:0x0000_0022 */
#define SPI_INT_STA_RX_RDY (0x1 << 0)												   /* rxFIFO ready, 0:RX_WL < RX_TRIG_LEVEL,1:RX_WL >= RX_TRIG_LEVEL */
#define SPI_INT_STA_RX_EMP (0x1 << 1)												   /* rxFIFO empty, this bit is set when rxFIFO is empty */
#define SPI_INT_STA_RX_FULL (0x1 << 2)												   /* rxFIFO full, this bit is set when rxFIFO is full */
#define SPI_INT_STA_TX_RDY (0x1 << 4)												   /* txFIFO ready, 0:TX_WL > TX_TRIG_LEVEL,1:TX_WL <= TX_TRIG_LEVEL */
#define SPI_INT_STA_TX_EMP (0x1 << 5)												   /* txFIFO empty, this bit is set when txFIFO is empty */
#define SPI_INT_STA_TX_FULL (0x1 << 6)												   /* txFIFO full, this bit is set when txFIFO is full */
#define SPI_INT_STA_RX_OVF (0x1 << 8)												   /* rxFIFO overflow, when set rxFIFO has overflowed */
#define SPI_INT_STA_RX_UDR (0x1 << 9)												   /* rxFIFO underrun, when set rxFIFO has underrun */
#define SPI_INT_STA_TX_OVF (0x1 << 10)												   /* txFIFO overflow, when set txFIFO has overflowed */
#define SPI_INT_STA_TX_UDR (0x1 << 11)												   /* fxFIFO underrun, when set txFIFO has underrun */
#define SPI_INT_STA_TC (0x1 << 12)													   /* Transfer Completed */
#define SPI_INT_STA_SSI (0x1 << 13)													   /* SS invalid interrupt, when set SS has changed from valid to invalid */
#define SPI_INT_STA_ERR (SPI_INT_STA_TX_OVF | SPI_INT_STA_RX_UDR | SPI_INT_STA_RX_OVF) /* NO txFIFO underrun */
#define SPI_INT_STA_MASK (0x77 | (0x3f << 8))
#define SPI_INT_STA_PENDING_BIT (0xffffffff)

/* SPI FIFO Control Register Bit Fields & Masks,default value:0x0040_0001 */
#define SPI_FIFO_CTL_RX_LEVEL (0xFF << 0)  /* rxFIFO reday request trigger level,default 0x1 */
#define SPI_FIFO_CTL_RX_DRQEN (0x1 << 8)   /* rxFIFO DMA request enable,1:enable,0:disable */
#define SPI_FIFO_CTL_RX_TESTEN (0x1 << 14) /* rxFIFO test mode enable,1:enable,0:disable */
#define SPI_FIFO_CTL_RX_RST (0x1 << 15)	   /* rxFIFO reset, write 1, auto clear to 0 */
#define SPI_FIFO_CTL_TX_LEVEL (0xFF << 16) /* txFIFO empty request trigger level,default 0x40 */
#define SPI_FIFO_CTL_TX_DRQEN (0x1 << 24)  /* txFIFO DMA request enable,1:enable,0:disable */
#define SPI_FIFO_CTL_TX_TESTEN (0x1 << 30) /* txFIFO test mode enable,1:enable,0:disable */
#define SPI_FIFO_CTL_TX_RST (0x1 << 31)	   /* txFIFO reset, write 1, auto clear to 0 */
#define SPI_FIFO_CTL_DRQEN_MASK (SPI_FIFO_CTL_TX_DRQEN | SPI_FIFO_CTL_RX_DRQEN)

/* SPI FIFO Status Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_FIFO_STA_RX_CNT (0xFF << 0)	 /* rxFIFO counter,how many bytes in rxFIFO */
#define SPI_FIFO_STA_RB_CNT (0x7 << 12)	 /* rxFIFO read buffer counter,how many bytes in rxFIFO read buffer */
#define SPI_FIFO_STA_RB_WR (0x1 << 15)	 /* rxFIFO read buffer write enable */
#define SPI_FIFO_STA_TX_CNT (0xFF << 16) /* txFIFO counter,how many bytes in txFIFO */
#define SPI_FIFO_STA_TB_CNT (0x7 << 28)	 /* txFIFO write buffer counter,how many bytes in txFIFO write buffer */
#define SPI_FIFO_STA_TB_WR (0x1 << 31)	 /* txFIFO write buffer write enable */
#define SPI_RXCNT_BIT_POS (0)
#define SPI_TXCNT_BIT_POS (16)

#define SPI_FIFO_CTL_SHIFT (0x4)

/* SPI Wait Clock Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_WAIT_WCC_MASK (0xFFFF << 0) /* used only in master mode: Wait Between Transactions */
#define SPI_WAIT_SWC_MASK (0xF << 16)	/* used only in master mode: Wait before start dual data transfer in dual SPI mode */

/* SPI Clock Control Register Bit Fields & Masks,default:0x0000_0002 */
#define SPI_CLK_CTL_CDR2 (0xFF << 0) /* Clock Divide Rate 2,master mode only : SPI_CLK = AHB_CLK/(2*(n+1)) */
#define SPI_CLK_CTL_CDR1 (0xF << 8)	 /* Clock Divide Rate 1,master mode only : SPI_CLK = AHB_CLK/2^n */
#define SPI_CLK_CTL_DRS (0x1 << 12)	 /* Divide rate select,default,0:rate 1;1:rate 2 */
#define SPI_CLK_SCOPE (SPI_CLK_CTL_CDR2 + 1)

/* SPI Master Burst Counter Register Bit Fields & Masks,default:0x0000_0000 */
/* master mode: when SMC = 1,BC specifies total burst number, Max length is 16Mbytes */
#define SPI_BC_CNT_MASK (0xFFFFFF << 0) /* Total Burst Counter, tx length + rx length ,SMC=1 */

/* SPI Master Transmit Counter reigster default:0x0000_0000 */
#define SPI_TC_CNT_MASK (0xFFFFFF << 0) /* Write Transmit Counter, tx length, NOT rx length!!! */

/* SPI Master Burst Control Counter reigster Bit Fields & Masks,default:0x0000_0000 */
#define SPI_BCC_STC_MASK (0xFFFFFF << 0) /* master single mode transmit counter */
#define SPI_BCC_DBC_MASK (0xF << 24)	 /* master dummy burst counter */
#define SPI_BCC_DBC_POS (24)			 /* master dummy burst pos */
#define SPI_BCC_DUAL_MODE (0x1 << 28)	 /* master dual mode RX enable */
#define SPI_BCC_QUAD_MODE (0x1 << 29)	 /* master quad mode RX enable */

/* SPI Sample Delay Mode,default:0xaaaa_ffff */
#define SPI_SAMP_MODE_EN (1U << 2)
#define SPI_SAMP_DL_SW_EN (1U << 7)
#define DELAY_NORMAL_SAMPLE (0x100)
#define DELAY_0_5_CYCLE_SAMPLE (0x000)
#define DELAY_1_CYCLE_SAMPLE (0x010)
#define DELAY_1_5_CYCLE_SAMPLE (0x110)
#define DELAY_2_CYCLE_SAMPLE (0x101)
#define DELAY_2_5_CYCLE_SAMPLE (0x001)
#define DELAY_3_CYCLE_SAMPLE (0x011)
#define SAMP_MODE_DL_DEFAULT 0xaaaaffff

typedef struct {
	uint32_t volatile ver; /* version number register */
	uint32_t volatile gc;  /* global control register */
	uint32_t volatile tc;  /* transfer control register */
	uint32_t volatile rev_01[1];
	uint32_t volatile int_ctl;	/* interrupt control register */
	uint32_t volatile int_sta;	/* interrupt status register */
	uint32_t volatile fifo_ctl; /* fifo control register */
	uint32_t volatile fifo_sta; /* fifo status register */
	uint32_t volatile wait_cnt; /* wait clock counter register */
	uint32_t volatile clk_ctl;	/* clock rate control register */
	uint32_t volatile sdc;		/* sample delay control register */
	uint32_t volatile rev_02[1];
	uint32_t volatile burst_cnt;	/* burst counter register */
	uint32_t volatile transmit_cnt; /* transmit counter register */
	uint32_t volatile bcc;			/* burst control counter register */
	uint32_t volatile rev_03[19];
	uint32_t volatile dma_ctl; /* DMA control register */
	uint32_t volatile rev_04[93];
	uint32_t volatile txdata; /* tx data register */
	uint32_t volatile rev_05[63];
	uint32_t volatile rxdata; /* rx data register */
} sunxi_spi_reg_t;

#endif// __REG_SPI_H__