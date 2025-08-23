/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#include "sys-sdcard.h"
#include "sys-sdhci.h"

#define FALSE 0
#define TRUE 1
static void set_read_timeout(sdhci_t *sdhci, uint32_t timeout) {
	uint32_t rval = 0;
	uint32_t rdto_clk = 0;
	uint32_t mode_2x = 0;

	rdto_clk = sdhci->clock / 1000 * timeout;
	rval = sdhci->reg->ntsr;
	mode_2x = rval & (0x1 << 31);

	if ((sdhci->clock == MMC_CLK_50M_DDR && mode_2x)) {
		rdto_clk = rdto_clk << 1;
	}

	rval = sdhci->reg->gctrl;
	/*ddr50 mode don't use 256x timeout unit*/
	if (rdto_clk > 0xffffff && sdhci->clock == MMC_CLK_50M_DDR) {
		rdto_clk = (rdto_clk + 255) / 256;
		rval |= (0x1 << 11);
	} else {
		rdto_clk = 0xffffff;
		rval &= ~(0x1 << 11);
	}
	sdhci->reg->gctrl = rval;

	rval = sdhci->reg->timeout;
	rval &= ~(0xffffff << 8);
	rval |= (rdto_clk << 8);
	sdhci->reg->timeout = rval;

	printk_trace("rdtoclk:%u, reg-tmout:%u, gctl:%x, clock:%u, nstr:%x\n", rdto_clk, sdhci->reg->timeout, sdhci->reg->gctrl, sdhci->clock, sdhci->reg->ntsr);
}

static int prepare_dma(sdhci_t *sdhci, sdhci_data_t *data) {
	sdhci_idma_desc_t *pdes = sdhci->dma_desc;
	uint32_t byte_cnt = data->blksz * data->blkcnt;
	uint8_t *buff;
	uint32_t des_idx = 0;
	uint32_t buff_frag_num = 0;
	uint32_t remain;
	uint32_t i;

	buff = data->buf;
	buff_frag_num = byte_cnt >> SMHC_DES_NUM_SHIFT;
	remain = byte_cnt & (SMHC_DES_BUFFER_MAX_LEN - 1);
	if (remain)
		buff_frag_num++;
	else
		remain = SMHC_DES_BUFFER_MAX_LEN - 1;

	for (i = 0; i < buff_frag_num; i++, des_idx++) {
		memset((void *) &pdes[des_idx], 0, sizeof(sdhci_idma_desc_t));
		pdes[des_idx].des_chain = 1;
		pdes[des_idx].own = 1;
		pdes[des_idx].dic = 1;
		if (buff_frag_num > 1 && i != buff_frag_num - 1)
			pdes[des_idx].data_buf_sz = SMHC_DES_BUFFER_MAX_LEN - 1;
		else
			pdes[des_idx].data_buf_sz = remain;
		pdes[des_idx].buf_addr = ((uint32_t) buff + i * SMHC_DES_BUFFER_MAX_LEN) >> 2;
		if (i == 0)
			pdes[des_idx].first_desc = 1;

		if (i == buff_frag_num - 1) {
			pdes[des_idx].dic = 0;
			pdes[des_idx].last_desc = 1;
			pdes[des_idx].next_desc_addr = 0;
		} else {
			pdes[des_idx].next_desc_addr = ((uint32_t) &pdes[des_idx + 1]) >> 2;
		}
		printk_trace("SMHC: frag %d, remain %d, des[%d] = 0x%08x:"
					 "  [0] = 0x%08x, [1] = 0x%08x, [2] = 0x%08x, [3] = 0x%08x\n",
					 i, remain, des_idx, (uint32_t) (&pdes[des_idx]), (uint32_t) ((uint32_t *) &pdes[des_idx])[0], (uint32_t) ((uint32_t *) &pdes[des_idx])[1],
					 (uint32_t) ((uint32_t *) &pdes[des_idx])[2], (uint32_t) ((uint32_t *) &pdes[des_idx])[3]);
	}

	wmb();

	/*
	 * GCTRLREG
	 * GCTRL[2]	: DMA reset
	 * GCTRL[5]	: DMA enable
	 *
	 * IDMACREG
	 * IDMAC[0]	: IDMA soft reset
	 * IDMAC[1]	: IDMA fix burst flag
	 * IDMAC[7]	: IDMA on
	 *
	 * IDIECREG
	 * IDIE[0]	: IDMA transmit interrupt flag
	 * IDIE[1]	: IDMA receive interrupt flag
	 */
	sdhci->reg->idst = 0x337;										   // clear interrupt status
	sdhci->reg->gctrl |= SMHC_GCTRL_DMA_ENABLE | SMHC_GCTRL_DMA_RESET; /* dma enable */
	sdhci->reg->dmac = SMHC_IDMAC_SOFT_RESET;						   /* idma reset */
	while (sdhci->reg->dmac & SMHC_IDMAC_SOFT_RESET)
		;

	sdhci->reg->dmac = SMHC_IDMAC_FIX_BURST | SMHC_IDMAC_IDMA_ON; /* idma on */
	sdhci->reg->idie &= ~(SMHC_IDMAC_TRANSMIT_INTERRUPT | SMHC_IDMAC_RECEIVE_INTERRUPT);
	if (data->flag & MMC_DATA_WRITE)
		sdhci->reg->idie |= SMHC_IDMAC_TRANSMIT_INTERRUPT;
	else
		sdhci->reg->idie |= SMHC_IDMAC_RECEIVE_INTERRUPT;

	sdhci->reg->dlba = (uint32_t) pdes >> 2;
	sdhci->reg->ftrglevel = sdhci->dma_trglvl;

	return 0;
}

static int wait_done(sdhci_t *sdhci, sdhci_data_t *dat, uint32_t timeout_msecs, uint32_t flag, bool dma) {
	uint32_t status;
	uint32_t done = 0;
	uint32_t start = time_ms();
	printk_trace("SMHC: wait for flag 0x%x\n", flag);
	do {
		status = sdhci->reg->rint;
		if ((time_ms() > (start + timeout_msecs))) {
			printk_warning("SMHC: wait timeout %x status %x flag %x\n", status & SMHC_RINT_INTERRUPT_ERROR_BIT, status, flag);
			return -1;
		} else if ((status & SMHC_RINT_INTERRUPT_ERROR_BIT)) {
			printk_warning("SMHC: error 0x%x status 0x%x\n", status & SMHC_RINT_INTERRUPT_ERROR_BIT, status & ~SMHC_RINT_INTERRUPT_ERROR_BIT);
			return -1;
		}
		if (dat && dma && (dat->blkcnt * dat->blksz) > 0)
			done = ((status & flag) && (sdhci->reg->idst & SMHC_IDMAC_RECEIVE_INTERRUPT)) ? 1 : 0;
		else
			done = (status & flag);
	} while (!done);

	return 0;
}

static bool read_bytes(sdhci_t *sdhci, sdhci_data_t *dat) {
	uint32_t count = dat->blkcnt * dat->blksz;
	uint32_t *tmp = (uint32_t *) dat->buf;
	uint32_t status, err, done;
	uint32_t timeout = time_ms() + count;
	uint32_t in_fifo;

	if (timeout < 250)
		timeout = 250;

	printk_trace("SMHC: read %u\n", count);

	status = sdhci->reg->status;
	err = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	if (err)
		printk_warning("SMHC: interrupt error 0x%x status 0x%x\n", err & SMHC_RINT_INTERRUPT_ERROR_BIT, status);

	while ((!err) && (count >= sizeof(sdhci->reg->fifo))) {
		while (sdhci->reg->status & SMHC_STATUS_FIFO_EMPTY) {
			if (time_ms() > timeout) {
				printk_warning("SMHC: read timeout\n");
				return FALSE;
			}
		}
		in_fifo = SMHC_STATUS_FIFO_LEVEL(status);
		count -= sizeof(sdhci->reg->fifo) * in_fifo;
		while (in_fifo--) { *(tmp++) = sdhci->reg->fifo; }

		status = sdhci->reg->status;
		err = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	}

	do {
		status = sdhci->reg->rint;

		err = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
		if (dat->blkcnt > 1)
			done = status & SMHC_RINT_AUTO_COMMAND_DONE;
		else
			done = status & SMHC_RINT_DATA_OVER;

	} while (!done && !err);

	if (err & SMHC_RINT_INTERRUPT_ERROR_BIT) {
		printk_warning("SMHC: interrupt error 0x%x status 0x%x\n", err & SMHC_RINT_INTERRUPT_ERROR_BIT, status);
		return FALSE;
	}

	if (count) {
		printk_warning("SMHC: read %u leftover\n", count);
		return FALSE;
	}
	return TRUE;
}

static bool write_bytes(sdhci_t *sdhci, sdhci_data_t *dat) {
	uint64_t count = dat->blkcnt * dat->blksz;
	uint32_t *tmp = (uint32_t *) dat->buf;
	uint32_t status, err, done;
	uint32_t timeout = time_ms() + count;

	if (timeout < 250)
		timeout = 250;

	printk_trace("SMHC: write %u\n", count);

	status = sdhci->reg->status;
	err = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	while (!err && count) {
		while (sdhci->reg->status & SMHC_STATUS_FIFO_FULL) {
			if (time_ms() > timeout) {
				printk_warning("SMHC: write timeout\n");
				return FALSE;
			}
		}
		sdhci->reg->fifo = *(tmp++);
		count -= sizeof(uint32_t);

		status = sdhci->reg->status;
		err = sdhci->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
	}

	do {
		status = sdhci->reg->rint;
		err = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
		if (dat->blkcnt > 1)
			done = status & SMHC_RINT_AUTO_COMMAND_DONE;
		else
			done = status & SMHC_RINT_DATA_OVER;
	} while (!done && !err);

	if (err & SMHC_RINT_INTERRUPT_ERROR_BIT)
		return FALSE;

	if (count)
		return FALSE;
	return TRUE;
}

bool sdhci_transfer(sdhci_t *sdhci, sdhci_cmd_t *cmd, sdhci_data_t *dat) {
	uint32_t cmdval = 0;
	uint32_t status = 0;
	uint32_t timeout;
	bool dma = false;

	printk_trace("SMHC: CMD%u 0x%x dlen:%u\n", cmd->idx, cmd->arg, dat ? dat->blkcnt * dat->blksz : 0);

	if (cmd->idx == MMC_STOP_TRANSMISSION) {
		timeout = time_ms();
		do {
			status = sdhci->reg->status;
			if (time_ms() - timeout > 10) {
				sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
				sdhci->reg->rint = 0xffffffff;
				printk_warning("SMHC: stop timeout\n");
				return FALSE;
			}
		} while (status & SMHC_STATUS_CARD_DATA_BUSY);
		return TRUE;
	}

	if (cmd->idx == MMC_GO_IDLE_STATE)
		cmdval |= SMHC_CMD_SEND_INIT_SEQUENCE;

	if (cmd->resptype & MMC_RSP_PRESENT) {
		cmdval |= SMHC_CMD_RESP_EXPIRE;
		if (cmd->resptype & MMC_RSP_136)
			cmdval |= SMHC_CMD_LONG_RESPONSE;
		if (cmd->resptype & MMC_RSP_CRC)
			cmdval |= SMHC_CMD_CHECK_RESPONSE_CRC;
	}

	if (dat) {
		sdhci->reg->blksz = dat->blksz;
		sdhci->reg->bytecnt = (uint32_t) (dat->blkcnt * dat->blksz);

		cmdval |= SMHC_CMD_DATA_EXPIRE | SMHC_CMD_WAIT_PRE_OVER;
		set_read_timeout(sdhci, DTO_MAX);

		if (dat->flag & MMC_DATA_WRITE) {
			cmdval |= SMHC_CMD_WRITE;
			sdhci->reg->idst |= SMHC_IDMAC_TRANSMIT_INTERRUPT;// clear TX status
		}
		if (dat->flag & MMC_DATA_READ)
			sdhci->reg->idst |= SMHC_IDMAC_RECEIVE_INTERRUPT;// clear RX status
	}

	if (cmd->idx == MMC_WRITE_MULTIPLE_BLOCK || cmd->idx == MMC_READ_MULTIPLE_BLOCK)
		cmdval |= SMHC_CMD_SEND_AUTO_STOP;

	sdhci->reg->rint = 0xffffffff;// Clear status
	sdhci->reg->arg = cmd->arg;

	if (dat && (dat->blkcnt * dat->blksz) > 64) {
		dma = true;
		sdhci->reg->gctrl &= ~SMHC_GCTRL_ACCESS_BY_AHB;
		prepare_dma(sdhci, dat);
		sdhci->reg->cmd = cmdval | cmd->idx | SMHC_CMD_START;// Start
	} else if (dat && (dat->blkcnt * dat->blksz) > 0) {
		sdhci->reg->gctrl |= SMHC_GCTRL_ACCESS_BY_AHB;
		sdhci->reg->cmd = cmdval | cmd->idx | SMHC_CMD_START;// Start
		if (dat->flag & MMC_DATA_READ && !read_bytes(sdhci, dat))
			return FALSE;
		else if (dat->flag & MMC_DATA_WRITE && !write_bytes(sdhci, dat))
			return FALSE;
	} else {
		sdhci->reg->gctrl |= SMHC_GCTRL_ACCESS_BY_AHB;
		sdhci->reg->cmd = cmdval | cmd->idx | SMHC_CMD_START;// Start
	}

	if (wait_done(sdhci, 0, 100, SMHC_RINT_COMMAND_DONE, false)) {
		printk_warning("SMHC: cmd timeout\n");
		return FALSE;
	}

	if (dat && wait_done(sdhci, dat, 8000, dat->blkcnt > 1 ? SMHC_RINT_AUTO_COMMAND_DONE : SMHC_RINT_DATA_OVER, dma)) {
		printk_warning("SMHC: data timeout\n");
		return FALSE;
	}

	if (cmd->resptype & MMC_RSP_BUSY) {
		timeout = time_ms();
		do {
			status = sdhci->reg->status;
			if (time_ms() - timeout > 10) {
				sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
				sdhci->reg->rint = 0xffffffff;
				printk_warning("SMHC: busy timeout\n");
				return FALSE;
			}
		} while (status & SMHC_STATUS_CARD_DATA_BUSY);
	}

	if (cmd->resptype & MMC_RSP_136) {
		cmd->response[0] = sdhci->reg->resp3;
		cmd->response[1] = sdhci->reg->resp2;
		cmd->response[2] = sdhci->reg->resp1;
		cmd->response[3] = sdhci->reg->resp0;
	} else {
		cmd->response[0] = sdhci->reg->resp0;
	}

	// Cleanup and disable IDMA
	if (dat && dma) {
		status = sdhci->reg->idst;
		sdhci->reg->idst = status;
		sdhci->reg->idie = 0;
		sdhci->reg->dmac = 0;
		sdhci->reg->gctrl &= ~SMHC_GCTRL_DMA_ENABLE;
	}

	return TRUE;
}

bool sdhci_reset(sdhci_t *sdhci) {
	sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
	return TRUE;
}

bool sdhci_set_width(sdhci_t *sdhci, uint32_t width) {
	const char *mode = "1 bit";
	sdhci->reg->gctrl &= ~SMHC_GCTRL_DDR_MODE;
	switch (width) {
		case MMC_BUS_WIDTH_1:
			sdhci->reg->width = SMHC_WIDTH_1BIT;
			break;
		case MMC_BUS_WIDTH_4:
			sdhci->reg->width = SMHC_WIDTH_4BIT;
			mode = "4 bit";
			break;
		case MMC_BUS_WIDTH_8:
			sdhci->reg->width = SMHC_WIDTH_8BIT;
			mode = "8 bit";
			break;
		default:
			printk_error("SMHC: %u width value invalid\n", width);
			return FALSE;
	}
	if (sdhci->clock == MMC_CLK_50M_DDR) {
		sdhci->reg->gctrl |= SMHC_GCTRL_DDR_MODE;
		mode = "4 bit DDR";
	}

	printk_trace("SMHC: set width to %s\n", mode);
	return TRUE;
}

static int init_default_timing(sdhci_t *sdhci) {
	sdhci->odly[MMC_CLK_400K] = TM5_OUT_PH180;
	sdhci->odly[MMC_CLK_25M] = TM5_OUT_PH180;
	sdhci->odly[MMC_CLK_50M] = TM5_OUT_PH180;
	sdhci->odly[MMC_CLK_50M_DDR] = TM5_OUT_PH90;

	sdhci->sdly[MMC_CLK_400K] = TM5_IN_PH180;
	sdhci->sdly[MMC_CLK_25M] = TM5_IN_PH180;
	sdhci->sdly[MMC_CLK_50M] = TM5_IN_PH90;
	sdhci->sdly[MMC_CLK_50M_DDR] = TM5_IN_PH180;
	return 0;
}

static int config_delay(sdhci_t *sdhci) {
	uint32_t rval, freq, val;
	uint8_t odly, sdly;

	freq = sdhci->clock;

	odly = sdhci->odly[freq];
	sdly = sdhci->sdly[freq];

	printk_trace("SMHC: odly: %d   sldy: %d\n", odly, sdly);

	val = read32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)));
	val &= (~CCU_MMC_CTRL_ENABLE);
	write32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)), val);

	sdhci->reg->drv_dl &= (~(0x3 << 16));
	sdhci->reg->drv_dl |= (((odly & 0x1) << 16) | ((odly & 0x1) << 17));

	val = read32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)));
	val |= CCU_MMC_CTRL_ENABLE;
	write32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)), val);

	rval = sdhci->reg->ntsr;
	rval &= (~(0x3 << 8));
	rval |= ((sdly & 0x3) << 8);
	sdhci->reg->ntsr = rval;

	/* enable hw skew auto mode */
	if (sdhci->skew_auto_mode) {
		rval = sdhci->reg->skew_ctrl;
		rval |= (0x1 << 4);
		sdhci->reg->skew_ctrl = rval;
	}

	return 0;
}

static bool update_card_clock(sdhci_t *sdhci) {
	sdhci->reg->cmd = SMHC_CMD_START | SMHC_CMD_UPCLK_ONLY | SMHC_CMD_WAIT_PRE_OVER;
	uint32_t timeout = time_ms();

	do {
		if (time_ms() - timeout > 10)
			return FALSE;
	} while (sdhci->reg->cmd & SMHC_CMD_START);

	sdhci->reg->rint = 0xffffffff;
	return TRUE;
}

bool sdhci_set_clock(sdhci_t *sdhci, smhc_clk_t clock) {
	uint32_t div, n, mod_hz, pll, pll_hz, hz, val;

	switch (clock) {
		case MMC_CLK_400K:
			hz = 400000;
			break;
		case MMC_CLK_25M:
			hz = 25000000;
			break;
		case MMC_CLK_50M:
		case MMC_CLK_50M_DDR:
			hz = 50000000;
			break;
		case MMC_CLK_100M:
			hz = 100000000;
			break;
		case MMC_CLK_150M:
			hz = 150000000;
			break;
		case MMC_CLK_200M:
			hz = 200000000;
			break;
		default:
			printk_error("SHMC: invalid clock: %u\n", clock);
			return false;
	}

	if (hz < 1000000) {
		printk_trace("SMHC: set clock to %.2fKHz\n", (f32) ((f32) hz / 1000.0));
	} else {
		printk_trace("SMHC: set clock to %.2fMHz\n", (f32) ((f32) hz / 1000000.0));
	}

	if (sdhci->clock == MMC_CLK_50M_DDR)
		mod_hz = hz * 4; /* 4xclk: DDR 4(HS); */
	else
		mod_hz = hz * 2; /* 2xclk: SDR 1/4; */

	if (mod_hz <= 24000000) {
		pll = CCU_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		pll = sdhci->sdhci_pll;
		pll_hz = sunxi_clk_get_peri1x_rate();
	}

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div++;

	n = 0;
	while (div > 16) {
		n++;
		div = (div + 1) / 2;
	}

	if (n > 3) {
		printk_error("mmc %u error cannot set clock to %uHz\n", sdhci->name, hz);
		return false;
	}

	printk_trace("SMHC: clock ratio %u\n", div);

	sdhci->reg->clkcr &= ~SMHC_CLKCR_CARD_CLOCK_ON;// Disable clock
	if (!update_card_clock(sdhci))
		return false;

	sdhci->reg->ntsr |= SUNXI_MMC_NTSR_MODE_SEL_NEW;

	val = read32(CCU_BASE + CCU_SMHC_BGR_REG);
	if (sdhci->id == 2)
		val |= CCU_MMC_BGR_SMHC2_RST;
	else
		val |= CCU_MMC_BGR_SMHC0_RST;
	write32(CCU_BASE + CCU_SMHC_BGR_REG, val);

	val = read32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)));
	val &= (~CCU_MMC_CTRL_ENABLE);
	write32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)), val);

	val = read32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)));
	val = pll | CCU_MMC_CTRL_N(n) | CCU_MMC_CTRL_M(div);
	write32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)), val);

	val = read32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)));
	val |= CCU_MMC_CTRL_ENABLE;
	write32(CCU_BASE + (CCU_SMHC0_CLK_REG + (0x4 * sdhci->id)), val);

	val = read32(CCU_BASE + CCU_SMHC_BGR_REG);
	if (sdhci->id == 2)
		val |= CCU_MMC_BGR_SMHC2_GATE;
	else
		val |= CCU_MMC_BGR_SMHC0_GATE;
	write32(CCU_BASE + CCU_SMHC_BGR_REG, val);

	sdhci->pclk = mod_hz;

	sdhci->reg->clkcr |= SMHC_CLKCR_MASK_D0;// Mask D0 when updating
	sdhci->reg->clkcr &= ~(0xff);			// Clear div (set to 1)
	if (sdhci->clock == MMC_CLK_50M_DDR) {
		sdhci->reg->clkcr |= SMHC_CLKCR_CLOCK_DIV(2);
	}
	sdhci->reg->clkcr |= SMHC_CLKCR_CARD_CLOCK_ON;// Enable clock

	if (!update_card_clock(sdhci))
		return false;

	config_delay(sdhci);

	return true;
}

int sunxi_sdhci_init(sdhci_t *sdhci) {

	/* Check controller id correct */
	if (sdhci->id > MMC_CONTROLLER_2) {
		printk_error("SMHC: Unsupported MAX Controller reached\n");
		return -1;
	}

	sunxi_gpio_init(sdhci->gpio_clk.pin, sdhci->gpio_clk.mux);
	sunxi_gpio_set_pull(sdhci->gpio_clk.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_cmd.pin, sdhci->gpio_cmd.mux);
	sunxi_gpio_set_pull(sdhci->gpio_cmd.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci->gpio_d0.pin, sdhci->gpio_d0.mux);
	sunxi_gpio_set_pull(sdhci->gpio_d0.pin, GPIO_PULL_UP);

	/* Init GPIO for 4bit MMC */
	if (sdhci->width > MMC_BUS_WIDTH_1) {
		sunxi_gpio_init(sdhci->gpio_d1.pin, sdhci->gpio_d1.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d1.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci->gpio_d2.pin, sdhci->gpio_d2.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d2.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci->gpio_d3.pin, sdhci->gpio_d3.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d3.pin, GPIO_PULL_UP);
	}

	/* Init GPIO for 8bit MMC */
	if (sdhci->width > MMC_BUS_WIDTH_4) {
		sunxi_gpio_init(sdhci->gpio_d4.pin, sdhci->gpio_d4.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d4.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci->gpio_d5.pin, sdhci->gpio_d5.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d5.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci->gpio_d6.pin, sdhci->gpio_d6.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d6.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci->gpio_d7.pin, sdhci->gpio_d7.mux);
		sunxi_gpio_set_pull(sdhci->gpio_d7.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci->gpio_ds.pin, sdhci->gpio_ds.mux);
		sunxi_gpio_set_pull(sdhci->gpio_ds.pin, GPIO_PULL_DOWN);

		sunxi_gpio_init(sdhci->gpio_rst.pin, sdhci->gpio_rst.mux);
		sunxi_gpio_set_pull(sdhci->gpio_rst.pin, GPIO_PULL_UP);
	}

	init_default_timing(sdhci);
	sdhci_set_clock(sdhci, MMC_CLK_400K);

	sdhci->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
	sdhci->reg->rint = 0xffffffff;

	sdhci->dma_trglvl = ((0x3 << 28) | (15 << 16) | 240);

	udelay(100);

	return 0;
}
