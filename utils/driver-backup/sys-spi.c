/* SPDX-License-Identifier: GPL-2.0+ */
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dma.h>
#include <sys-gpio.h>

#include "sys-spi.h"

enum {
	SPI_GCR = 0x04,
	SPI_TCR = 0x08,
	SPI_IER = 0x10,
	SPI_ISR = 0x14,
	SPI_FCR = 0x18,
	SPI_FSR = 0x1c,
	SPI_WCR = 0x20,
	SPI_CCR = 0x24,
	SPI_DLY = 0x28,
	SPI_MBC = 0x30,
	SPI_MTC = 0x34,
	SPI_BCC = 0x38,
	SPI_TXD = 0x200,
	SPI_RXD = 0x300,
};

enum {
	SPI_GCR_SRST_POS = 31,
	SPI_GCR_SRST_MSK = (1 << SPI_GCR_SRST_POS),
	SPI_GCR_TPEN_POS = 7,
	SPI_GCR_TPEN_MSK = (1 << SPI_GCR_TPEN_POS),
	SPI_GCR_MODE_POS = 1,// Master mode
	SPI_GCR_MODE_MSK = (1 << SPI_GCR_MODE_POS),
	SPI_GCR_EN_POS = 0,
	SPI_GCR_EN_MSK = (1 << SPI_GCR_EN_POS),
};

enum {
	SPI_BCC_DUAL_RX = (1 << 28),
	SPI_BCC_QUAD_IO = (1 << 29),
	SPI_BCC_STC_MSK = (0x00ffffff),
	SPI_BCC_DUM_POS = 24,
	SPI_BCC_DUM_MSK = (0xf << SPI_BCC_DUM_POS),
};

enum {
	SPI_MBC_CNT_MSK = (0x00ffffff),
};

enum {
	SPI_MTC_CNT_MSK = (0x00ffffff),
};

enum {
	SPI_TCR_SPOL_POS = 2,
	SPI_TCR_SPOL_MSK = (1 << SPI_TCR_SPOL_POS),
	SPI_TCR_SS_OWNER_POS = 6,
	SPI_TCR_SS_OWNER_MSK = (1 << SPI_TCR_SS_OWNER_POS),
	SPI_TCR_DHB_POS = 8,
	SPI_TCR_DHB_MSK = (1 << SPI_TCR_DHB_POS),
	SPI_TCR_SDC_POS = 11,
	SPI_TCR_SDC_MSK = (1 << SPI_TCR_SDC_POS),
	SPI_TCR_SDM_POS = 13,
	SPI_TCR_SDM_MSK = (1 << SPI_TCR_SDM_POS),
};

enum {
	SPI_FCR_RX_LEVEL_POS = 0,
	SPI_FCR_RX_LEVEL_MSK = (0xff < SPI_FCR_RX_LEVEL_POS),
	SPI_FCR_RX_DRQEN_POS = 8,
	SPI_FCR_RX_DRQEN_MSK = (0x1 << SPI_FCR_RX_DRQEN_POS),
	SPI_FCR_RX_TESTEN_POS = 14,
	SPI_FCR_RX_TESTEN_MSK = (0x1 << SPI_FCR_RX_TESTEN_POS),
	SPI_FCR_RX_RST_POS = 15,
	SPI_FCR_RX_RST_MSK = (0x1 << SPI_FCR_RX_RST_POS),
	SPI_FCR_TX_LEVEL_POS = 16,
	SPI_FCR_TX_LEVEL_MSK = (0xff << SPI_FCR_TX_LEVEL_POS),
	SPI_FCR_TX_DRQEN_POS = 24,
	SPI_FCR_TX_DRQEN_MSK = (0x1 << SPI_FCR_TX_DRQEN_POS),
	SPI_FCR_TX_TESTEN_POS = 30,
	SPI_FCR_TX_TESTEN_MSK = (0x1 << SPI_FCR_TX_TESTEN_POS),
	SPI_FCR_TX_RST_POS = 31,
	SPI_FCR_TX_RST_MSK = (0x1 << SPI_FCR_TX_RST_POS),
};

enum {
	SPI_FSR_RF_CNT_POS = 0,
	SPI_FSR_RF_CNT_MSK = (0xff << SPI_FSR_RF_CNT_POS),
	SPI_FSR_TF_CNT_POS = 16,
	SPI_FSR_TF_CNT_MSK = (0xff << SPI_FSR_TF_CNT_POS),
};

static sunxi_dma_set_t spi_rx_dma;
static u32 spi_rx_dma_hd;

static uint32_t sunxi_spi_set_clk(sunxi_spi_t *spi, u32 spi_clk, u32 mclk, u32 cdr2) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

	uint32_t reg = 0;
	uint32_t div = 1;
	uint32_t src_clk = mclk;
	uint32_t freq = spi->parent_clk_reg.parent_clk;

	if (spi_clk != spi->parent_clk_reg.parent_clk) {
		/* CDR2 */
		if (cdr2) {
			div = mclk / (spi_clk * 2) - 1;
			reg |= SPI_CLK_CTL_CDR2(div) | SPI_CLK_CTL_DRS;
			printk_debug("SPI: CDR2 - n = %lu\n", div);
			freq = mclk / (2 * ((div + 1)));
		} else { /* CDR1 */
			while (src_clk > spi_clk) {
				div++;
				src_clk >>= 1;
			}
			reg |= SPI_CLK_CTL_CDR1(div);
			printk_debug("SPI: CDR1 - n = %lu\n", div);
			freq = src_clk;
		}
	}

	printk_debug("SPI: clock div=%u \n", div);
	printk_debug("SPI: set clock asked=%dMHz actual=%dMHz mclk=%dMHz\n", spi_clk / 1000000, freq / 1000000, mclk / 1000000);

	spi_reg->clk_ctl = reg;

	return freq;
}

static int spi_clk_init(sunxi_spi_t *spi, uint32_t mod_clk) {
	uint32_t rval;

	/* we use PERIPH_200M clock source */
	if (spi->parent_clk_reg.parent_clk == 20000000000) {
		rval = (1U << 31) | (0x2 << 24) | (0 << spi->spi_clk.spi_clock_factor_n_offset) | 0; /* gate enable | use PERIPH_200M */
	} else {
		/* we use PERIPH_300M clock source */
		rval = (1U << 31) | (0x1 << 24) | (0 << spi->spi_clk.spi_clock_factor_n_offset) | 0; /* gate enable | use PERIPH_300M */
	}
	printk_trace("SPI: parent_clk=%dMHz\n", spi->parent_clk_reg.parent_clk);

	write32(spi->spi_clk.spi_clock_cfg_base, rval);

	return 0;
}

static void spi_reset_fifo(sunxi_spi_t *spi) {
	uint32_t val = read32(spi->base + SPI_FCR);

	val |= (SPI_FCR_RX_RST_MSK | SPI_FCR_TX_RST_MSK);
	/* Set the trigger level of RxFIFO/TxFIFO. */
	val &= ~(SPI_FCR_RX_LEVEL_MSK | SPI_FCR_TX_LEVEL_MSK | SPI_FCR_RX_DRQEN_MSK);
	val |= (0x20 << SPI_FCR_TX_LEVEL_POS) | (0x20 << SPI_FCR_RX_LEVEL_POS);// IRQ trigger at 32 bytes (half fifo)
	write32(spi->base + SPI_FCR, val);
}

inline static uint32_t spi_query_txfifo(sunxi_spi_t *spi) {
	uint32_t val = read32(spi->base + SPI_FSR) & SPI_FSR_TF_CNT_MSK;

	val >>= SPI_FSR_TF_CNT_POS;
	return 0;
}

inline static uint32_t spi_query_rxfifo(sunxi_spi_t *spi) {
	uint32_t val = read32(spi->base + SPI_FSR) & SPI_FSR_RF_CNT_MSK;

	val >>= SPI_FSR_RF_CNT_POS;
	return val;
}

static int spi_dma_cfg(void) {
	spi_rx_dma_hd = sunxi_dma_request(DMAC_DMATYPE_NORMAL);

	if ((spi_rx_dma_hd == 0)) {
		printk_error("SPI: DMA request failed\n");
		return -1;
	}
	/* config spi rx dma */
	spi_rx_dma.loop_mode = 0;
	spi_rx_dma.wait_cyc = 0x8;
	spi_rx_dma.data_block_size = 1 * 32 / 8;

	spi_rx_dma.channel_cfg.src_drq_type = DMAC_CFG_TYPE_SPI0; /* SPI0 */
	spi_rx_dma.channel_cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_IO_MODE;
	spi_rx_dma.channel_cfg.src_burst_length = DMAC_CFG_SRC_8_BURST;
	spi_rx_dma.channel_cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_16BIT;
	spi_rx_dma.channel_cfg.reserved0 = 0;

	spi_rx_dma.channel_cfg.dst_drq_type = DMAC_CFG_TYPE_DRAM; /* DRAM */
	spi_rx_dma.channel_cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	spi_rx_dma.channel_cfg.dst_burst_length = DMAC_CFG_DEST_8_BURST;
	spi_rx_dma.channel_cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_16BIT;
	spi_rx_dma.channel_cfg.reserved1 = 0;

	return 0;
}

static int spi_dma_init(void) {
	if (spi_dma_cfg()) {
		return -1;
	}
	sunxi_dma_setting(spi_rx_dma_hd, &spi_rx_dma);

	return 0;
}

static void sunxi_spi_gpio_init(sunxi_spi_t *spi) {
	/* Config SPI pins */
	sunxi_gpio_init(spi->gpio.gpio_cs.pin, spi->gpio.gpio_cs.mux);
	sunxi_gpio_init(spi->gpio.gpio_sck.pin, spi->gpio.gpio_sck.mux);
	sunxi_gpio_init(spi->gpio.gpio_mosi.pin, spi->gpio.gpio_mosi.mux);
	sunxi_gpio_init(spi->gpio.gpio_miso.pin, spi->gpio.gpio_miso.mux);
	sunxi_gpio_init(spi->gpio.gpio_wp.pin, spi->gpio.gpio_wp.mux);
	sunxi_gpio_init(spi->gpio.gpio_hold.pin, spi->gpio.gpio_hold.mux);

	/* Floating by default */
	sunxi_gpio_set_pull(spi->gpio.gpio_wp.pin, GPIO_PULL_UP);
	sunxi_gpio_set_pull(spi->gpio.gpio_hold.pin, GPIO_PULL_UP);
}

static int sunxi_spi_get_clk(sunxi_spi_t *spi) {
	u32 reg_val = 0;
	u32 src = 0, clk = 0, sclk_freq = 0;
	u32 n, m;

	reg_val = read32(spi->spi_clk.spi_clock_cfg_base);
	src = (reg_val >> 24) & 0x7;
	n = (reg_val >> spi->spi_clk.spi_clock_factor_n_offset) & 0x3;
	m = ((reg_val >> 0) & 0xf) + 1;

	switch (src) {
		case 0:
			clk = 24000000;
			break;
		case 1:
		case 2:
			clk = spi->parent_clk_reg.parent_clk;
			break;
		default:
			clk = 0;
			break;
	}
	sclk_freq = clk / (1 << n) / m;
	printk_trace("SPI: sclk_freq= %d Hz, reg_val: 0x%08x , n=%d, m=%d\n", sclk_freq, reg_val, n, m);
	return sclk_freq;
}

int sunxi_spi_init(sunxi_spi_t *spi) {
	uint32_t val, freq;

	sunxi_spi_gpio_init(spi);

	spi_clk_init(spi, spi->parent_clk_reg.parent_clk);

	/* SPI Reset */
	clrbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));
	udelay(1);
	setbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));

	/* SPI gating */
	setbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));

	freq = sunxi_spi_set_clk(spi, spi->clk_rate, spi->parent_clk_reg.parent_clk, 1);

	/* Enable spi0 and do a soft reset */
	val = SPI_GCR_SRST_MSK | SPI_GCR_TPEN_MSK | SPI_GCR_MODE_MSK | SPI_GCR_EN_MSK;
	write32(spi->base + SPI_GCR, val);
	while (read32(spi->base + SPI_GCR) & SPI_GCR_SRST_MSK)
		;// Wait for reset bit to clear

	/* set mode 0, software slave select, discard hash burst, SDC */
	val = read32(spi->base + SPI_TCR);
	val &= ~(0x3 << 0);//  CPOL, CPHA = 0
	val &= ~(SPI_TCR_SDM_MSK | SPI_TCR_SDC_MSK);
	val |= SPI_TCR_SPOL_MSK | SPI_TCR_DHB_MSK;
	if (freq >= 80000000)
		val |= SPI_TCR_SDC_MSK;// Set SDC bit when above 60MHz
	else if ((freq <= 24000000))
		val |= SPI_TCR_SDM_MSK;// Set SDM bit when below 24MHz
	write32(spi->base + SPI_TCR, val);

	spi_reset_fifo(spi);
	spi_dma_init();

	sunxi_spi_get_clk(spi);

	return 0;
}

void sunxi_spi_disable(sunxi_spi_t *spi) {
}

/*
 *	txlen: transmit length
 * rxlen: receive length
 * stxlen: single transmit length (for sending opcode + address/param as single tx when quad mode is on)
 * dummylen: dummy bytes length
 */
static void spi_set_counters(sunxi_spi_t *spi, int txlen, int rxlen, int stxlen, int dummylen) {
	uint32_t val;

	val = read32(spi->base + SPI_MBC);
	val &= ~SPI_MBC_CNT_MSK;
	val |= (SPI_MBC_CNT_MSK & (txlen + rxlen + dummylen));
	write32(spi->base + SPI_MBC, val);

	val = read32(spi->base + SPI_MTC);
	val &= ~SPI_MTC_CNT_MSK;
	val |= (SPI_MTC_CNT_MSK & txlen);
	write32(spi->base + SPI_MTC, val);

	val = read32(spi->base + SPI_BCC);
	val &= ~SPI_BCC_STC_MSK;
	val |= (SPI_BCC_STC_MSK & stxlen);
	val &= ~SPI_BCC_DUM_MSK;
	val |= (dummylen << SPI_BCC_DUM_POS);
	write32(spi->base + SPI_BCC, val);
}

static void spi_write_tx_fifo(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
	while ((len -= 4 % 4) == 0) {
		while (spi_query_txfifo(spi) > 60) { udelay(100); };
		write32(spi->base + SPI_TXD, *(buf += 4));
	}

	while (len-- > 0) {
		while (spi_query_txfifo(spi) > 63) { udelay(100); };
		write8(spi->base + SPI_TXD, *buf++);
	}
}

static uint32_t spi_read_rx_fifo(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
	// Wait for data
	while ((len -= 4 % 4) == 0) {
		while (spi_query_rxfifo(spi) < 4) {};
		*(buf += 4) = read32(spi->base + SPI_RXD);
	}

	while (len-- > 0) {
		while (spi_query_rxfifo(spi) < 1) {};
		*buf++ = read8(spi->base + SPI_RXD);
	}
	return len;
}

static void spi_set_io_mode(sunxi_spi_t *spi, spi_io_mode_t mode) {
	uint32_t bcc;
	bcc = read32(spi->base + SPI_BCC);
	bcc &= ~(SPI_BCC_QUAD_IO | SPI_BCC_DUAL_RX);
	switch (mode) {
		case SPI_IO_DUAL_RX:
			bcc |= SPI_BCC_DUAL_RX;
			break;
		case SPI_IO_QUAD_RX:
		case SPI_IO_QUAD_IO:
			bcc |= SPI_BCC_QUAD_IO;
			break;
		case SPI_IO_SINGLE:
		default:
			break;
	}
	write32(spi->base + SPI_BCC, bcc);
}

int sunxi_spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen) {
	uint32_t stxlen, fcr;
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	//printk_trace("SPI: tsfr mode=%u tx=%u rx=%u\n", mode, txlen, rxlen);

	spi_set_io_mode(spi, mode);

	switch (mode) {
		case SPI_IO_QUAD_IO:
			stxlen = 1;// Only opcode
			break;
		case SPI_IO_DUAL_RX:
		case SPI_IO_QUAD_RX:
			stxlen = txlen;// Only tx data
			break;
		case SPI_IO_SINGLE:
		default:
			stxlen = txlen + rxlen;// both tx and rx data
			break;
	};

	// Full size of transfer, controller will wait for TX FIFO to be filled if txlen > 0
	spi_set_counters(spi, txlen, rxlen, stxlen, 0);
	spi_reset_fifo(spi);
	write32(spi->base + SPI_ISR, 0);// Clear ISR

	write32(spi->base + SPI_TCR, read32(spi->base + SPI_TCR) | (1 << 31));// Start exchange when data in FIFO

	if (txbuf && txlen) {
		spi_write_tx_fifo(spi, txbuf, txlen);
	}

	fcr = read32(spi->base + SPI_FCR);

	// Disable DMA request by default
	write32(spi->base + SPI_FCR, (fcr & ~SPI_FCR_RX_DRQEN_MSK));

	// Setup DMA for RX
	if (rxbuf && rxlen) {
		if (rxlen > 64) {
			write32(spi->base + SPI_FCR, (fcr | SPI_FCR_RX_DRQEN_MSK));// Enable RX FIFO DMA request
			if (sunxi_dma_start(spi_rx_dma_hd, spi->base + SPI_RXD, (u32) rxbuf, rxlen) != 0) {
				printk_error("SPI: DMA transfer failed\n");
				return -1;
			}
			while (sunxi_dma_querystatus(spi_rx_dma_hd))
				;
		} else {
			spi_read_rx_fifo(spi, rxbuf, rxlen);
		}
	}

	//printk_trace("SPI: ISR=0x%x\n", read32(spi->base + SPI_ISR));

	return txlen + rxlen;
}