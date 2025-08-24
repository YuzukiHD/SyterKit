/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <sys-dma.h>

#ifndef SUNXI_DMA_MAX
#define SUNXI_DMA_MAX 4
#endif

static int dma_int_cnt = 0;

static int dma_init_ok = -1;

static sunxi_dma_source_t dma_channel_source[SUNXI_DMA_MAX];

static sunxi_dma_desc_t dma_channel_desc[SUNXI_DMA_MAX] __attribute__((aligned(64)));

static uint32_t DMA_REG_BASE = 0x0;

void sunxi_dma_clk_init(sunxi_dma_t *dma) {
	/* DMA : mbus clock gating */
	setbits_le32(dma->bus_clk.gate_reg_base, BIT(dma->bus_clk.gate_reg_offset));
	/* DMA reset */
	setbits_le32(dma->dma_clk.rst_reg_base, BIT(dma->dma_clk.rst_reg_offset));
	/* DMA gating */
	setbits_le32(dma->dma_clk.gate_reg_base, BIT(dma->dma_clk.gate_reg_offset));
}

void sunxi_dma_init(sunxi_dma_t *dma) {
	int i = 0;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) dma->dma_reg_base;

	DMA_REG_BASE = dma->dma_reg_base;

	if (dma_init_ok > 0)
		return;

	sunxi_dma_clk_init(dma);

	dma_reg->irq_en0 = 0;
	dma_reg->irq_en1 = 0;

	dma_reg->irq_pending0 = 0xffffffff;
	dma_reg->irq_pending1 = 0xffffffff;

	/* auto MCLK gating disable */
	dma_reg->auto_gate &= ~(0x7 << 0);
	dma_reg->auto_gate |= 0x7 << 0;

	memset((void *) dma_channel_source, 0, SUNXI_DMA_MAX * sizeof(sunxi_dma_source_t));

	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		dma_channel_source[i].used = 0;
		dma_channel_source[i].channel = &(dma_reg->channel[i]);
		dma_channel_source[i].desc = &dma_channel_desc[i];
	}

	dma_int_cnt = 0;
	dma_init_ok = 1;

	return;
}

void sunxi_dma_exit(sunxi_dma_t *dma) {
	uint32_t dma_fd;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) dma->dma_reg_base;

	/* free dma channel if other module not free it */
	for (int i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma_channel_source[i].used == 1) {
			dma_channel_source[i].channel->enable = 0;
			dma_channel_source[i].used = 0;
			dma_fd = (uint32_t) &dma_channel_source[i];
			sunxi_dma_disable_int(dma_fd);
			sunxi_dma_free_int(dma_fd);
		}
	}

	/* close dma clock when dma exit */
	dma_reg->auto_gate &= ~(1 << dma->dma_clk.gate_reg_offset | 1 << dma->dma_clk.rst_reg_offset);

	dma_reg->irq_en0 = 0;
	dma_reg->irq_en1 = 0;

	dma_reg->irq_pending0 = 0xffffffff;
	dma_reg->irq_pending1 = 0xffffffff;

	dma_init_ok--;
}

uint32_t sunxi_dma_request_from_last(uint32_t dmatype) {
	for (int i = SUNXI_DMA_MAX - 1; i >= 0; i--) {
		if (dma_channel_source[i].used == 0) {
			dma_channel_source[i].used = 1;
			dma_channel_source[i].channel_count = i;
			return (uint32_t) &dma_channel_source[i];
		}
	}

	return 0;
}

uint32_t sunxi_dma_request(uint32_t dmatype) {
	for (int i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma_channel_source[i].used == 0) {
			dma_channel_source[i].used = 1;
			dma_channel_source[i].channel_count = i;
			printk_debug("DMA: provide channel %u\n", i);
			return (uint32_t) &dma_channel_source[i];
		}
	}

	return 0;
}

int sunxi_dma_release(uint32_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;

	if (!dma_source->used) {
		return -1;
	}

	sunxi_dma_disable_int(dma_fd);
	sunxi_dma_free_int(dma_fd);

	dma_source->used = 0;

	return 0;
}

int sunxi_dma_setting(uint32_t dma_fd, sunxi_dma_set_t *cfg) {
	uint32_t commit_para;
	sunxi_dma_set_t *dma_set = cfg;
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_desc_t *desc = dma_source->desc;
	uint32_t channel_addr = (uint32_t) (&(dma_set->channel_cfg));

	if (!dma_source->used)
		return -1;

	if (dma_set->loop_mode)
		desc->link = (uint32_t) (&dma_source->desc);
	else
		desc->link = SUNXI_DMA_LINK_NULL;

	commit_para = (dma_set->wait_cyc & 0xff);
	commit_para |= (dma_set->data_block_size & 0xff) << 8;

	desc->commit_para = commit_para;
	desc->config = *(volatile uint32_t *) channel_addr;

	return 0;
}

int sunxi_dma_start(uint32_t dma_fd, uint32_t saddr, uint32_t daddr, uint32_t bytes) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_channel_reg_t *channel = dma_source->channel;
	sunxi_dma_desc_t *desc = dma_source->desc;

	if (!dma_source->used)
		return -1;

	/*config desc */
	desc->source_addr = saddr;
	desc->dest_addr = daddr;
	desc->byte_count = bytes;

	/* start dma */
	channel->desc_addr = (uint32_t) desc;
	channel->enable = 1;

	return 0;
}

int sunxi_dma_stop(uint32_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_channel_reg_t *channel = dma_source->channel;

	if (!dma_source->used)
		return -1;
	channel->enable = 0;

	return 0;
}

int sunxi_dma_querystatus(uint32_t dma_fd) {
	uint32_t channel_count;
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) DMA_REG_BASE;

	if (!dma_source->used)
		return -1;

	channel_count = dma_source->channel_count;

	return (dma_reg->status >> channel_count) & 0x01;
}

int sunxi_dma_install_int(uint32_t dma_fd, void *p) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) DMA_REG_BASE;
	uint8_t channel_count;

	if (!dma_source->used)
		return -1;

	channel_count = dma_source->channel_count;

	if (channel_count < 8)
		dma_reg->irq_pending0 = (7 << channel_count * 4);
	else
		dma_reg->irq_pending1 = (7 << (channel_count - 8) * 4);

	if (!dma_source->dma_func.m_func) {
		dma_source->dma_func.m_func = 0;
		dma_source->dma_func.m_data = p;
	} else {
		printk_error("DMA: 0x%08x int is used already, you have to free it first\n", dma_fd);
	}

	return 0;
}

int sunxi_dma_enable_int(uint32_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) DMA_REG_BASE;
	uint8_t channel_count;

	if (!dma_source->used)
		return -1;

	channel_count = dma_source->channel_count;
	if (channel_count < 8) {
		if ((dma_reg->irq_en0) & (DMA_PKG_END_INT << channel_count * 4)) {
			printk_debug("DMA: 0x%08x int is avaible already\n", dma_fd);
			return 0;
		}
		dma_reg->irq_en0 |= (DMA_PKG_END_INT << channel_count * 4);
	} else {
		if ((dma_reg->irq_en1) & (DMA_PKG_END_INT << (channel_count - 8) * 4)) {
			printk_debug("DMA: 0x%08x int is avaible already\n", dma_fd);
			return 0;
		}
		dma_reg->irq_en1 |= (DMA_PKG_END_INT << (channel_count - 8) * 4);
	}

	dma_int_cnt++;

	return 0;
}

int sunxi_dma_disable_int(uint32_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) DMA_REG_BASE;
	uint8_t channel_count;

	if (!dma_source->used)
		return -1;

	channel_count = dma_source->channel_count;
	if (channel_count < 8) {
		if (!((dma_reg->irq_en0) & (DMA_PKG_END_INT << channel_count * 4))) {
			printk_debug("DMA: 0x%08x int is not used yet\n", dma_fd);
			return 0;
		}
		dma_reg->irq_en0 &= ~(DMA_PKG_END_INT << channel_count * 4);
	} else {
		if (!((dma_reg->irq_en1) & (DMA_PKG_END_INT << (channel_count - 8) * 4))) {
			printk_debug("DMA: 0x%08x int is not used yet\n", dma_fd);
			return 0;
		}
		dma_reg->irq_en1 &= ~(DMA_PKG_END_INT << (channel_count - 8) * 4);
	}

	/* disable golbal int */
	if (dma_int_cnt > 0)
		dma_int_cnt--;

	return 0;
}

int sunxi_dma_free_int(uint32_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg = (sunxi_dma_reg_t *) DMA_REG_BASE;
	uint8_t channel_count;

	if (!dma_source->used)
		return -1;

	channel_count = dma_source->channel_count;
	if (channel_count < 8)
		dma_reg->irq_pending0 = (7 << channel_count);
	else
		dma_reg->irq_pending1 = (7 << (channel_count - 8));

	if (dma_source->dma_func.m_func) {
		dma_source->dma_func.m_func = NULL;
		dma_source->dma_func.m_data = NULL;
	} else {
		printk_debug("DMA: 0x%08x int is free, you do not need to free it again\n", dma_fd);
		return -1;
	}

	return 0;
}


int sunxi_dma_test(uint32_t *src_addr, uint32_t *dst_addr, uint32_t len) {
	sunxi_dma_set_t dma_set;
	uint32_t st = 0;
	uint32_t timeout;
	uint32_t dma_fd;
	uint32_t i, valid;

	len = ALIGN(len, 4);
	printk_debug("DMA: test 0x%08x ====> 0x%08x, len %uKB \n", (uint32_t) src_addr, (uint32_t) dst_addr, (len / 1024));

	/* dma */
	dma_set.loop_mode = 0;
	dma_set.wait_cyc = 8;
	dma_set.data_block_size = 1 * 32 / 8;
	/* channel config (from dram to dram)*/
	dma_set.channel_cfg.src_drq_type = DMAC_CFG_TYPE_DRAM;// dram
	dma_set.channel_cfg.src_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	dma_set.channel_cfg.src_burst_length = DMAC_CFG_SRC_4_BURST;
	dma_set.channel_cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_16BIT;
	dma_set.channel_cfg.reserved0 = 0;

	dma_set.channel_cfg.dst_drq_type = DMAC_CFG_TYPE_DRAM;// dram
	dma_set.channel_cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	dma_set.channel_cfg.dst_burst_length = DMAC_CFG_DEST_4_BURST;
	dma_set.channel_cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_16BIT;
	dma_set.channel_cfg.reserved1 = 0;

	dma_fd = sunxi_dma_request(0);
	if (!dma_fd) {
		printk_error("DMA: can't request dma\n");
		return -1;
	}

	sunxi_dma_setting(dma_fd, &dma_set);

	// prepare data
	for (i = 0; i < (len / 4); i += 4) {
		src_addr[i] = i;
		src_addr[i + 1] = i + 1;
		src_addr[i + 2] = i + 2;
		src_addr[i + 3] = i + 3;
	}

	/* timeout : 100 ms */
	timeout = time_ms();

	sunxi_dma_start(dma_fd, (uint32_t) src_addr, (uint32_t) dst_addr, len);
	st = sunxi_dma_querystatus(dma_fd);

	while ((time_ms() - timeout < 100) && st) { st = sunxi_dma_querystatus(dma_fd); }

	if (st) {
		printk_error("DMA: test timeout!\n");
		sunxi_dma_stop(dma_fd);
		sunxi_dma_release(dma_fd);

		return -2;
	} else {
		valid = 1;
		printk_debug("DMA: test done in %lums\n", (time_ms() - timeout));
		// Check data is valid
		for (i = 0; i < (len / 4); i += 4) {
			if (dst_addr[i] != i || dst_addr[i + 1] != i + 1 || dst_addr[i + 2] != i + 2 || dst_addr[i + 3] != i + 3) {
				valid = 0;
				break;
			}
		}
		if (valid)
			printk_debug("DMA: test check valid\n");
		else
			printk_error("DMA: test check failed at %u bytes\n", i);
	}

	sunxi_dma_stop(dma_fd);
	sunxi_dma_release(dma_fd);

	return 0;
}