/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <dt2c/driver.h>
#include <log.h>
#include <string.h>
#include <timer.h>

#include "spif-internal.h"

#ifdef CONFIG_ARCH_DCACHE
#include <cache.h>
#endif

static inline uintptr_t sunxi_spif_reg(const sunxi_spif_t *spif, uint32_t offset)
{
	return spif->base + offset;
}

static void sunxi_spif_cache_clean(uintptr_t start, uint32_t size)
{
#ifdef CONFIG_ARCH_DCACHE
	if (size != 0U)
		flush_dcache_range(start, start + size);
#else
	(void)start;
	(void)size;
#endif
}

static void sunxi_spif_cache_invalidate(uintptr_t start, uint32_t size)
{
#ifdef CONFIG_ARCH_DCACHE
	if (size != 0U)
		invalidate_dcache_range(start, start + size);
#else
	(void)start;
	(void)size;
#endif
}

static void sunxi_spif_clock_disable(const sunxi_spif_t *spif)
{
	if (spif->clock_reg != 0U)
		clrbits_le32(spif->clock_reg, BIT(31));
	if (spif->clk.gate_reg_base != 0U)
		clrbits_le32(spif->clk.gate_reg_base, BIT(spif->clk.gate_reg_offset));
	if (spif->clk.rst_reg_base != 0U)
		clrbits_le32(spif->clk.rst_reg_base, BIT(spif->clk.rst_reg_offset));
}

static void sunxi_spif_clock_enable(const sunxi_spif_t *spif)
{
	if (spif->clk.rst_reg_base != 0U)
		setbits_le32(spif->clk.rst_reg_base, BIT(spif->clk.rst_reg_offset));
	if (spif->clk.gate_reg_base != 0U)
		setbits_le32(spif->clk.gate_reg_base, BIT(spif->clk.gate_reg_offset));
}

static int sunxi_spif_wait_clear(const sunxi_spif_t *spif, uint32_t offset, uint32_t mask)
{
	uint32_t timeout = SPIF_TIMEOUT;

	while ((readl(sunxi_spif_reg(spif, offset)) & mask) != 0U) {
		if (--timeout == 0U)
			return -1;
	}
	return 0;
}

static int sunxi_spif_soft_reset(const sunxi_spif_t *spif)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GCA_REG));

	writel(value | SPIF_GCA_DMA_END, sunxi_spif_reg(spif, SPIF_GCA_REG));
	if (sunxi_spif_wait_clear(spif, SPIF_GCA_REG, SPIF_GCA_DMA_END) != 0)
		return -1;

	writel(value | SPIF_GCA_SOFT_SRST, sunxi_spif_reg(spif, SPIF_GCA_REG));
	return sunxi_spif_wait_clear(spif, SPIF_GCA_REG, SPIF_GCA_SOFT_SRST);
}

static int sunxi_spif_fifo_reset(const sunxi_spif_t *spif)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GCA_REG));

	writel(value | SPIF_GCA_RF_SRST | SPIF_GCA_WF_SRST, sunxi_spif_reg(spif, SPIF_GCA_REG));
	return sunxi_spif_wait_clear(spif, SPIF_GCA_REG, SPIF_GCA_RF_SRST | SPIF_GCA_WF_SRST);
}

static void sunxi_spif_set_mode(const sunxi_spif_t *spif, uint32_t mode)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));

	value &= ~(SPIF_GC_CPHA | SPIF_GC_CPOL);
	value |= mode & (SPIF_GC_CPHA | SPIF_GC_CPOL);
	writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
}

static int sunxi_spif_set_cs(const sunxi_spif_t *spif, uint8_t chip_select)
{
	uint32_t value;

	if (chip_select > 3U)
		return -1;
	value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));
	value &= ~SPIF_GC_SS_MASK;
	value |= (uint32_t)chip_select << 6;
	value |= SPIF_GC_CS_POL;
	writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
	return 0;
}

static void sunxi_spif_set_sample(const sunxi_spif_t *spif)
{
	uint32_t value;

	if (spif->sample_mode == SUNXI_SPIF_SAMPLE_DEFAULT || spif->sample_delay == SUNXI_SPIF_SAMPLE_DEFAULT)
		return;
	value = readl(sunxi_spif_reg(spif, SPIF_TC_REG));
	value |= SPIF_TC_DIGITAL_ANALOG_EN | SPIF_TC_ANALOG_DL_SW_RX_EN;
	value &= ~(SPIF_TC_DIGITAL_DELAY_MASK | SPIF_TC_ANALOG_DELAY_MASK);
	value |= (spif->sample_mode << 16) & SPIF_TC_DIGITAL_DELAY_MASK;
	value |= spif->sample_delay & SPIF_TC_ANALOG_DELAY_MASK;
	writel(value, sunxi_spif_reg(spif, SPIF_TC_REG));
	mdelay(1);
}

static void sunxi_spif_set_dtr(const sunxi_spif_t *spif, bool enable)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));

	if (enable)
		value |= SPIF_GC_DTR_EN;
	else
		value &= ~SPIF_GC_DTR_EN;
	writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
}

static void sunxi_spif_set_dtr_clock(const sunxi_spif_t *spif, bool enable)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_TC_REG));

	if (enable)
		value |= SPIF_TC_CLK_SCKOUT_SRC_SEL;
	else
		value &= ~SPIF_TC_CLK_SCKOUT_SRC_SEL;
	writel(value, sunxi_spif_reg(spif, SPIF_TC_REG));
}

static bool sunxi_spif_is_dtr_opcode(uint8_t opcode)
{
	return opcode == SPIF_DTR_READ_1_1_1 || opcode == SPIF_DTR_READ_1_2_2 || opcode == SPIF_DTR_READ_1_4_4 ||
	       opcode == SPIF_DTR_READ_1_1_1_4B || opcode == SPIF_DTR_READ_1_2_2_4B || opcode == SPIF_DTR_READ_1_4_4_4B;
}

static int sunxi_spif_program_clock(sunxi_spif_t *spif, uint32_t speed_hz)
{
	uint32_t best_n = 0U;
	uint32_t best_m = 1U;
	uint32_t best_rate = 0U;
	uint64_t best_error = 0xffffffffffffffffULL;
	uint32_t fallback_n = 0U;
	uint32_t fallback_m = 1U;
	uint32_t fallback_rate = 0U;
	uint32_t n_limit;
	uint32_t m_limit;
	uint32_t n;
	uint32_t m;
	bool have_floor = false;

	if (spif->clock_reg == 0U || spif->clock_parent_hz == 0U || speed_hz == 0U)
		return -1;

	if (spif->clock_layout == SUNXI_SPIF_CLOCK_LAYOUT_DIV2) {
		n_limit = 4U;
		m_limit = 16U;
	} else {
		n_limit = 4U;
		m_limit = 32U;
	}

	for (n = 0U; n < n_limit; ++n) {
		for (m = 1U; m <= m_limit; ++m) {
			uint32_t rate;
			uint64_t error;

			rate = (uint32_t)((uint64_t)spif->clock_parent_hz / (((uint64_t)1U << n) * m));
			if (rate == 0U)
				continue;
			if (rate <= speed_hz) {
				error = (uint64_t)speed_hz - rate;
				if (!have_floor || error < best_error) {
					best_error = error;
					best_rate = rate;
					best_n = n;
					best_m = m;
					have_floor = true;
				}
			} else if (!have_floor && (fallback_rate == 0U || rate < fallback_rate)) {
				fallback_rate = rate;
				fallback_n = n;
				fallback_m = m;
			}
		}
	}

	if (!have_floor) {
		best_rate = fallback_rate;
		best_n = fallback_n;
		best_m = fallback_m;
	}
	if (best_rate == 0U)
		return -1;

	if (spif->clock_layout == SUNXI_SPIF_CLOCK_LAYOUT_DIV2) {
		writel(BIT(31) | ((spif->clock_source & 3U) << 24) | (best_n << 16) | (best_m - 1U), spif->clock_reg);
	} else {
		writel(BIT(31) | ((spif->clock_source & 7U) << 24) | (best_n << spif->clock_n_offset) | (best_m - 1U),
			spif->clock_reg);
	}
	spif->actual_speed_hz = best_rate;
	printk_debug("SPIF: requested=%u actual=%u parent=%u n=%u m=%u\n", speed_hz, best_rate, spif->clock_parent_hz,
		best_n, best_m);
	return 0;
}

static int sunxi_spif_claim_bus(sunxi_spif_t *spif)
{
	uint32_t value;

	if (sunxi_spif_soft_reset(spif) != 0 || sunxi_spif_fifo_reset(spif) != 0)
		return -1;

	value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));
	value &= ~(SPIF_GC_NMODE_EN | SPIF_GC_PMODE_EN | SPIF_GC_CFG_MODE | SPIF_GC_RX_CFG_FBS | SPIF_GC_TX_CFG_FBS |
		   SPIF_GC_DTR_EN | SPIF_GC_WP_EN | SPIF_GC_HOLD_EN);
	writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
	value = readl(sunxi_spif_reg(spif, SPIF_TC_REG));
	value &= ~(SPIF_TC_CLK_SCKOUT_SRC_SEL);
	writel(value, sunxi_spif_reg(spif, SPIF_TC_REG));

	if (sunxi_spif_set_cs(spif, spif->chip_select) != 0)
		return -1;
	sunxi_spif_set_mode(spif, 0U);
	sunxi_spif_set_sample(spif);
	value = readl(sunxi_spif_reg(spif, SPIF_CSD_REG));
	value = (value & ~0x00ffffffU) | SPIF_CSD_DEFAULT;
	writel(value, sunxi_spif_reg(spif, SPIF_CSD_REG));
	return 0;
}

static int sunxi_spif_reconfigure_clock(sunxi_spif_t *spif, uint32_t speed_hz)
{
	sunxi_spif_clock_disable(spif);
	if (sunxi_spif_program_clock(spif, speed_hz) != 0) {
		sunxi_spif_clock_enable(spif);
		return -1;
	}
	sunxi_spif_clock_enable(spif);
	return sunxi_spif_claim_bus(spif);
}

int sunxi_spif_init(sunxi_spif_t *spif)
{
	uint32_t default_speed;
	uint32_t version;

	if (spif == NULL || spif->base == 0U || spif->clock_reg == 0U || spif->clock_parent_hz == 0U ||
		spif->clk.gate_reg_base == 0U || spif->clk.rst_reg_base == 0U)
		return -1;
	if (spif->min_speed_hz == 0U)
		spif->min_speed_hz = SUNXI_SPIF_MIN_FREQUENCY;
	if (spif->max_speed_hz == 0U)
		spif->max_speed_hz = SUNXI_SPIF_MAX_FREQUENCY;
	if (spif->bus_freq < spif->min_speed_hz || spif->bus_freq > spif->max_speed_hz)
		return -1;

	sunxi_gpio_init(&spif->gpio_cs);
	sunxi_gpio_init(&spif->gpio_sck);
	sunxi_gpio_init(&spif->gpio_mosi);
	if (spif->gpio_miso.base != 0U)
		sunxi_gpio_init(&spif->gpio_miso);
	if (spif->gpio_wp.base != 0U) {
		sunxi_gpio_init(&spif->gpio_wp);
		sunxi_gpio_set_pull(&spif->gpio_wp, GPIO_PULL_UP);
	}
	if (spif->gpio_hold.base != 0U) {
		sunxi_gpio_init(&spif->gpio_hold);
		sunxi_gpio_set_pull(&spif->gpio_hold, GPIO_PULL_UP);
	}

	default_speed = spif->bus_freq < SUNXI_SPIF_DEFAULT_FREQUENCY ? spif->bus_freq : SUNXI_SPIF_DEFAULT_FREQUENCY;
	sunxi_spif_clock_disable(spif);
	udelay(10);
	sunxi_spif_clock_enable(spif);
	version = readl(sunxi_spif_reg(spif, SPIF_VER_REG));
	if (sunxi_spif_reconfigure_clock(spif, default_speed) != 0) {
		sunxi_spif_clock_disable(spif);
		return -1;
	}
	spif->speed_hz = default_speed;
	spif->initialized = 1U;
	printk_info("SPIF: initialized version=0x%x base=%p clock=%uHz\n", version, (void *)spif->base,
		spif->actual_speed_hz);
	return 0;
}

void sunxi_spif_disable(sunxi_spif_t *spif)
{
	if (spif == NULL || spif->base == 0U)
		return;
	clrbits_le32(sunxi_spif_reg(spif, SPIF_GC_REG), SPIF_GC_NMODE_EN);
	sunxi_spif_clock_disable(spif);
	spif->initialized = 0U;
}

int sunxi_spif_select(sunxi_spif_t *spif, uint8_t chip_select)
{
	if (spif == NULL || spif->base == 0U || !spif->initialized)
		return -1;
	if (sunxi_spif_set_cs(spif, chip_select) != 0)
		return -1;
	spif->chip_select = chip_select;
	return 0;
}

int sunxi_spif_update_clk(sunxi_spif_t *spif, uint32_t speed_hz)
{
	uint32_t max_speed;

	if (spif == NULL || !spif->initialized || speed_hz == 0U)
		return -1;
	max_speed = spif->max_speed_hz * ((spif->rx_dtr_en || spif->tx_dtr_en) ? 2U : 1U);
	if (speed_hz < spif->min_speed_hz || speed_hz > max_speed)
		return -1;
	if (sunxi_spif_reconfigure_clock(spif, speed_hz) != 0)
		return -1;
	spif->speed_hz = speed_hz;
	return 0;
}

int sunxi_spif_set_config(sunxi_spif_t *spif, const struct spif_cfg *cfg)
{
	if (spif == NULL || cfg == NULL)
		return -1;
	if ((cfg->valid & SPIF_CFG_SPEED_HZ) != 0U && cfg->speed_hz == 0U)
		return -1;
	if ((cfg->valid & SPIF_CFG_RX_DTR) != 0U)
		spif->rx_dtr_en = cfg->rx_dtr_en != 0U;
	if ((cfg->valid & SPIF_CFG_TX_DTR) != 0U)
		spif->tx_dtr_en = cfg->tx_dtr_en != 0U;
	if ((cfg->valid & SPIF_CFG_SAMPLE_MODE) != 0U)
		spif->sample_mode = cfg->sample_mode;
	if ((cfg->valid & SPIF_CFG_SAMPLE_DELAY) != 0U)
		spif->sample_delay = cfg->sample_delay;
	if ((cfg->valid & SPIF_CFG_SPEED_HZ) != 0U) {
		if (sunxi_spif_update_clk(spif, cfg->speed_hz) != 0)
			return -1;
	}
	if (spif->initialized && (cfg->valid & (SPIF_CFG_SAMPLE_MODE | SPIF_CFG_SAMPLE_DELAY)) != 0U)
		sunxi_spif_set_sample(spif);
	return 0;
}

int sunxi_spif_transfer(sunxi_spif_t *spif, struct spif_descriptor_op *desc, uint32_t data_len)
{
	uint32_t timeout = SPIF_TIMEOUT;
	uint32_t descriptor_count;
	uint32_t descriptor_size;
	uint32_t max_transfer;
	uintptr_t data_addr;
	uint32_t block_len_mask;
	uint32_t descriptor_address;
	uint8_t opcode;
	uint64_t dtr_speed;
	bool dtr;
	int ret = 0;

	if (spif == NULL || desc == NULL || !spif->initialized)
		return -1;
	if (sunxi_spif_fifo_reset(spif) != 0)
		return -1;

	opcode = (uint8_t)(desc->cmd_mode_buswidth >> 24);
	dtr = sunxi_spif_is_dtr_opcode(opcode) &&
	      ((desc->trans_phase & SPIF_TRANS_RX_EN) != 0U ? spif->rx_dtr_en : spif->tx_dtr_en);
	if (dtr) {
		dtr_speed = (uint64_t)spif->speed_hz * 2U;
		if (dtr_speed > 0xffffffffULL || sunxi_spif_reconfigure_clock(spif, (uint32_t)dtr_speed) != 0) {
			ret = -1;
			goto restore_mode;
		}
		sunxi_spif_set_dtr_clock(spif, true);
		sunxi_spif_set_dtr(spif, true);
	} else {
		sunxi_spif_set_dtr_clock(spif, false);
		sunxi_spif_set_dtr(spif, false);
	}

	block_len_mask = sunxi_spif_platform_data_len_mask();
	if ((desc->block_data_len & block_len_mask) == 0U) {
		uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));

		value &= ~SPIF_GC_CFG_MODE;
		writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
		writel(desc->trans_phase, sunxi_spif_reg(spif, SPIF_PHC_REG));
		writel(desc->flash_addr, sunxi_spif_reg(spif, SPIF_TCF_REG));
		writel(desc->cmd_mode_buswidth, sunxi_spif_reg(spif, SPIF_TCS_REG));
		writel(desc->addr_dummy_data_count, sunxi_spif_reg(spif, SPIF_TNM_REG));
		value |= SPIF_GC_NMODE_EN;
		writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
		while ((readl(sunxi_spif_reg(spif, SPIF_GC_REG)) & SPIF_GC_NMODE_EN) != 0U) {
			if (--timeout == 0U) {
				ret = -1;
				goto restore_mode;
			}
		}
		goto restore_mode;
	}

	max_transfer = sunxi_spif_platform_max_transfer();
	descriptor_count = (data_len - 1U) / max_transfer + 1U;
	if (descriptor_count > 0xffffffffU / sizeof(*desc)) {
		ret = -1;
		goto restore_mode;
	}
	descriptor_size = descriptor_count * sizeof(*desc);
	if (sunxi_spif_platform_decode_data_addr(desc->data_addr, &data_addr) != 0) {
		ret = -1;
		goto restore_mode;
	}
	if ((uintptr_t)data_len > (uintptr_t)-1 - (uintptr_t)data_addr ||
		(uintptr_t)descriptor_size > (uintptr_t)-1 - (uintptr_t)desc) {
		ret = -1;
		goto restore_mode;
	}
	if (sunxi_spif_platform_encode_desc_addr((uintptr_t)desc, &descriptor_address) != 0) {
		ret = -1;
		goto restore_mode;
	}

	sunxi_spif_cache_clean((uintptr_t)data_addr, data_len);
	sunxi_spif_cache_clean((uintptr_t)desc, descriptor_size);
	setbits_le32(sunxi_spif_reg(spif, SPIF_GC_REG), SPIF_GC_CFG_MODE);
	writel((readl(sunxi_spif_reg(spif, SPIF_DMA_CTL_REG)) & ~0x000003f0U) | SPIF_DMA_DESCRIPTOR_LEN,
		sunxi_spif_reg(spif, SPIF_DMA_CTL_REG));
	writel(descriptor_address, sunxi_spif_reg(spif, SPIF_DSC_REG));
	writel(SPIF_DMA_DONE_INT, sunxi_spif_reg(spif, SPIF_INT_STA_REG));
	setbits_le32(sunxi_spif_reg(spif, SPIF_DMA_CTL_REG), SPIF_DMA_START);

	while ((readl(sunxi_spif_reg(spif, SPIF_INT_STA_REG)) & SPIF_DMA_DONE_INT) == 0U) {
		if (--timeout == 0U) {
			ret = -1;
			goto restore_mode;
		}
	}
	sunxi_spif_cache_invalidate((uintptr_t)data_addr, data_len);
	writel(SPIF_DMA_DONE_INT, sunxi_spif_reg(spif, SPIF_INT_STA_REG));

restore_mode:
	if (dtr) {
		sunxi_spif_set_dtr_clock(spif, false);
		sunxi_spif_set_dtr(spif, false);
		if (sunxi_spif_reconfigure_clock(spif, spif->speed_hz) != 0 && ret == 0)
			ret = -1;
	}
	return ret;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-spif");
