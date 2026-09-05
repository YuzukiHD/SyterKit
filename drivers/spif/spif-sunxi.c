/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "spif-sunxi: " fmt

/**
 * @file spif-sunxi.c
 * @brief Sunxi SPI Flash (SPIF) controller driver.
 *
 * Implements the descriptor-based DMA transfer engine for the SPIF
 * controller: clock programming, mode/sample configuration, descriptor
 * encoding, and both normal and DTR transfers, on top of the platform glue
 * defined in spif-platform.h.
 */

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <driver.h>
#include <dt2c/driver.h>
#include <drivers/spif/spif.h>
#include <log.h>
#include <malloc.h>
#include <string.h>
#include <timer.h>

#include "spif-regs.h"
#include "platform/spif-platform.h"

#ifdef CONFIG_ARCH_DCACHE
#include <cache.h>
#endif

#define DEBUG_SPIF_CLK 0

/** @struct spif_descriptor_op
 * @brief Hardware DMA descriptor for one SPIF transfer segment.
 */
struct spif_descriptor_op {
	u32 hburst_rw_flag;      /**< Burst direction and DMA flags. */
	u32 block_data_len;      /**< Data length encoded for the block. */
	u32 data_addr;           /**< Memory address of the data buffer. */
	u32 next_des_addr;       /**< Address of the next descriptor, or zero. */
	u32 trans_phase;         /**< Enabled command/address/mode/dummy phases. */
	u32 flash_addr;          /**< Target flash address for this segment. */
	u32 cmd_mode_buswidth;   /**< Command opcode and bus-width encodings. */
	u32 addr_dummy_data_count; /**< Address, dummy, and data count fields. */
};

/* SPI operation descriptor preparation. */
#define SUNXI_SPIF_DESC_COUNT 16U
#define SUNXI_SPIF_DESC_SIZE  (SUNXI_SPIF_DESC_COUNT * sizeof(struct spif_descriptor_op))

/** @struct sunxi_spif_op_buffers
 * @brief Descriptor and bounce-buffer allocations for one SPI operation.
 */
struct sunxi_spif_op_buffers {
	struct spif_descriptor_op *desc; /**< Aligned descriptor array. */
	void *desc_allocation;           /**< Raw allocation backing @p desc. */
	uint8_t *cache;                  /**< Aligned bounce cache, when used. */
	void *cache_allocation;          /**< Raw allocation backing @p cache. */
};

/* Controller register, clock, and mode helpers. */
/**
 * @brief Compute a SPIF controller register address.
 *
 * @param[in] spif SPIF controller descriptor.
 * @param[in] offset Register offset within the controller.
 * @return The absolute register address.
 */
static inline uintptr_t sunxi_spif_reg(const sunxi_spif_t *spif, uint32_t offset)
{
	return spif->base + offset;
}

/**
 * @brief Flush a data-cache range before a DMA transfer.
 *
 * @param[in] start Start address of the range.
 * @param[in] size Size of the range in bytes.
 */
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

/**
 * @brief Invalidate a data-cache range after a DMA transfer.
 *
 * @param[in] start Start address of the range.
 * @param[in] size Size of the range in bytes.
 */
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

/**
 * @brief Disable the SPIF controller clock and reset signals.
 *
 * @param[in] spif SPIF controller descriptor.
 */
static void sunxi_spif_clock_disable(const sunxi_spif_t *spif)
{
	if (spif->clock_reg != 0U)
		clrbits_le32(spif->clock_reg, BIT(31));
	if (spif->clk.gate_reg_base != 0U)
		clrbits_le32(spif->clk.gate_reg_base, BIT(spif->clk.gate_reg_offset));
	if (spif->clk.rst_reg_base != 0U)
		clrbits_le32(spif->clk.rst_reg_base, BIT(spif->clk.rst_reg_offset));
}

/**
 * @brief Enable the SPIF controller reset and clock signals.
 *
 * @param[in] spif SPIF controller descriptor.
 */
static void sunxi_spif_clock_enable(const sunxi_spif_t *spif)
{
	if (spif->clk.rst_reg_base != 0U)
		setbits_le32(spif->clk.rst_reg_base, BIT(spif->clk.rst_reg_offset));
	if (spif->clk.gate_reg_base != 0U)
		setbits_le32(spif->clk.gate_reg_base, BIT(spif->clk.gate_reg_offset));
}

/**
 * @brief Poll a controller register until the given bits are clear.
 *
 * @param[in] spif SPIF controller descriptor.
 * @param[in] offset Register offset to poll.
 * @param[in] mask Bit mask that must read zero.
 * @return 0 on success, -1 on timeout.
 */
static int sunxi_spif_wait_clear(const sunxi_spif_t *spif, uint32_t offset, uint32_t mask)
{
	uint32_t timeout = SPIF_TIMEOUT;

	while ((readl(sunxi_spif_reg(spif, offset)) & mask) != 0U) {
		if (--timeout == 0U)
			return -1;
	}
	return 0;
}

/**
 * @brief Perform a soft reset of the SPIF controller.
 *
 * @param[in] spif SPIF controller descriptor.
 * @return 0 on success, -1 on timeout.
 */
static int sunxi_spif_soft_reset(const sunxi_spif_t *spif)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GCA_REG));

	writel(value | SPIF_GCA_DMA_END, sunxi_spif_reg(spif, SPIF_GCA_REG));
	if (sunxi_spif_wait_clear(spif, SPIF_GCA_REG, SPIF_GCA_DMA_END) != 0)
		return -1;

	writel(value | SPIF_GCA_SOFT_SRST, sunxi_spif_reg(spif, SPIF_GCA_REG));
	return sunxi_spif_wait_clear(spif, SPIF_GCA_REG, SPIF_GCA_SOFT_SRST);
}

/**
 * @brief Reset the SPIF receive and transmit FIFOs.
 *
 * @param[in] spif SPIF controller descriptor.
 * @return 0 on success, -1 on timeout.
 */
static int sunxi_spif_fifo_reset(const sunxi_spif_t *spif)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GCA_REG));

	writel(value | SPIF_GCA_RF_SRST | SPIF_GCA_WF_SRST, sunxi_spif_reg(spif, SPIF_GCA_REG));
	return sunxi_spif_wait_clear(spif, SPIF_GCA_REG, SPIF_GCA_RF_SRST | SPIF_GCA_WF_SRST);
}

/**
 * @brief Program the SPI clock phase and polarity.
 *
 * @param[in] spif SPIF controller descriptor.
 * @param[in] mode Clock mode bits (CPHA/CPOL) to apply.
 */
static void sunxi_spif_set_mode(const sunxi_spif_t *spif, uint32_t mode)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));

	value &= ~(SPIF_GC_CPHA | SPIF_GC_CPOL);
	value |= mode & (SPIF_GC_CPHA | SPIF_GC_CPOL);
	writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
}

/**
 * @brief Select the active chip-select line.
 *
 * @param[in] spif SPIF controller descriptor.
 * @param[in] chip_select Chip-select index (0-3).
 * @return 0 on success, -1 when @p chip_select is out of range.
 */
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

/**
 * @brief Program the RX sample delay and mode for the controller.
 *
 * @param[in] spif SPIF controller descriptor.
 */
static void sunxi_spif_set_sample(const sunxi_spif_t *spif)
{
	uint32_t value;

	value = readl(sunxi_spif_reg(spif, SPIF_TC_REG));

	if (spif->sample_mode == SUNXI_SPIF_SAMPLE_DEFAULT || spif->sample_delay == SUNXI_SPIF_SAMPLE_DEFAULT) {
		value &= ~(SPIF_TC_DIGITAL_ANALOG_EN | SPIF_TC_ANALOG_DL_SW_RX_EN | SPIF_TC_DIGITAL_DELAY_MASK |
			   SPIF_TC_ANALOG_DELAY_MASK);
		writel(value, sunxi_spif_reg(spif, SPIF_TC_REG));
		return;
	}

	value |= SPIF_TC_DIGITAL_ANALOG_EN | SPIF_TC_ANALOG_DL_SW_RX_EN;
	value &= ~(SPIF_TC_DIGITAL_DELAY_MASK | SPIF_TC_ANALOG_DELAY_MASK);
	value |= (spif->sample_mode << 16) & SPIF_TC_DIGITAL_DELAY_MASK;
	value |= spif->sample_delay & SPIF_TC_ANALOG_DELAY_MASK;
	writel(value, sunxi_spif_reg(spif, SPIF_TC_REG));
	mdelay(1);
}

/**
 * @brief Enable or disable DTR mode.
 *
 * @param[in] spif SPIF controller descriptor.
 * @param[in] enable true to enable DTR, false to disable.
 */
static void sunxi_spif_set_dtr(const sunxi_spif_t *spif, bool enable)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_GC_REG));

	if (enable)
		value |= SPIF_GC_DTR_EN;
	else
		value &= ~SPIF_GC_DTR_EN;
	writel(value, sunxi_spif_reg(spif, SPIF_GC_REG));
}

/**
 * @brief Route the doubled DTR clock to the controller output.
 *
 * @param[in] spif SPIF controller descriptor.
 * @param[in] enable true to select the doubled clock, false for the normal one.
 */
static void sunxi_spif_set_dtr_clock(const sunxi_spif_t *spif, bool enable)
{
	uint32_t value = readl(sunxi_spif_reg(spif, SPIF_TC_REG));

	if (enable)
		value |= SPIF_TC_CLK_SCKOUT_SRC_SEL;
	else
		value &= ~SPIF_TC_CLK_SCKOUT_SRC_SEL;
	writel(value, sunxi_spif_reg(spif, SPIF_TC_REG));
}

/**
 * @brief Check whether an opcode uses DTR read addressing.
 *
 * @param[in] opcode Flash opcode to test.
 * @return true for the DTR 1-1-1 / 1-2-2 / 1-4-4 read opcodes.
 */
static bool sunxi_spif_is_dtr_opcode(uint8_t opcode)
{
	return opcode == SPIF_DTR_READ_1_1_1 || opcode == SPIF_DTR_READ_1_2_2 || opcode == SPIF_DTR_READ_1_4_4 ||
	       opcode == SPIF_DTR_READ_1_1_1_4B || opcode == SPIF_DTR_READ_1_2_2_4B || opcode == SPIF_DTR_READ_1_4_4_4B;
}

/**
 * @brief Program the SPIF clock divider for a requested frequency.
 *
 * Searches the (n, m) divider pairs for the closest rate at or below the
 * request, then writes the clock control register and records the achieved
 * frequency.
 *
 * @param[in,out] spif SPIF controller descriptor.
 * @param[in] speed_hz Desired bus frequency in Hz.
 * @return 0 on success, -1 when the clock cannot be configured.
 */
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
#if DEBUG_SPIF_CLK == 1
	pr_trace("requested=%u actual=%u parent=%u n=%u m=%u\n", speed_hz, best_rate, spif->clock_parent_hz,
		best_n, best_m);
#endif
	return 0;
}

/* Controller lifecycle and runtime configuration. */
/**
 * @brief Reset the controller and configure the bus for a transfer.
 *
 * @param[in,out] spif SPIF controller descriptor.
 * @return 0 on success, -1 on failure.
 */
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

/**
 * @brief Disable, reprogram, and re-enable the SPIF clock.
 *
 * @param[in,out] spif SPIF controller descriptor.
 * @param[in] speed_hz Desired bus frequency in Hz.
 * @return 0 on success, -1 on failure.
 */
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

/**
 * @brief Allocate a cache-line-aligned buffer.
 *
 * @param[in] size Requested buffer size in bytes.
 * @param[out] allocation Raw heap allocation to free later.
 * @param[out] aligned Cache-line-aligned pointer into the allocation.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID on failure.
 */
static int sunxi_spif_alloc_aligned(size_t size, void **allocation, void **aligned)
{
	uintptr_t address;
	void *raw;

	if (size == 0U || allocation == NULL || aligned == NULL || size > (size_t)-1 - (SUNXI_SPIF_CACHELINE_SIZE - 1U))
		return DRIVER_ERROR_INVALID;
	raw = malloc(size + SUNXI_SPIF_CACHELINE_SIZE - 1U);
	if (raw == NULL)
		return DRIVER_ERROR_INVALID;
	address = (uintptr_t)raw;
	if (address > (uintptr_t)-1 - (SUNXI_SPIF_CACHELINE_SIZE - 1U)) {
		free(raw);
		return DRIVER_ERROR_INVALID;
	}
	address = (address + SUNXI_SPIF_CACHELINE_SIZE - 1U) & ~(uintptr_t)(SUNXI_SPIF_CACHELINE_SIZE - 1U);
	*allocation = raw;
	*aligned = (void *)address;
	return DRIVER_OK;
}

/**
 * @brief Translate a SPIF bus-width constant to its hardware encoding.
 *
 * @param[in] buswidth SPIF_SINGLE/DUAL/QUAD/OCTAL_MODE constant.
 * @return The hardware width encoding, or DRIVER_ERROR_INVALID if unknown.
 */
static int sunxi_spif_buswidth(uint8_t buswidth)
{
	switch (buswidth) {
	case SPIF_SINGLE_MODE:
		return 0;
	case SPIF_DUAL_MODE:
		return 1;
	case SPIF_QUAD_MODE:
		return 2;
	case SPIF_OCTAL_MODE:
		return 3;
	default:
		return DRIVER_ERROR_INVALID;
	}
}

/**
 * @brief Compute a base-plus-offset address with overflow checking.
 *
 * @param[in] base Base address.
 * @param[in] offset Offset to add.
 * @param[out] result Receives the summed address on success.
 * @return 0 on success, DRIVER_ERROR_INVALID on overflow or null @p result.
 */
static int sunxi_spif_add_offset(uintptr_t base, uint32_t offset, uintptr_t *result)
{
	if (result == NULL || (uintptr_t)offset > (uintptr_t)-1 - base)
		return DRIVER_ERROR_INVALID;
	*result = base + offset;
	return 0;
}

/**
 * @brief Initialize a descriptor with the platform burst and length fields.
 *
 * @param[out] desc Descriptor to initialize.
 */
static void sunxi_spif_init_descriptor(struct spif_descriptor_op *desc)
{
	desc->hburst_rw_flag = sunxi_spif_platform_hburst_rw_flag();
	desc->block_data_len = sunxi_spif_platform_block_data_len();
	desc->addr_dummy_data_count = SPIF_DES_NORMAL_EN;
}

/**
 * @brief Encode the command, address, mode, and dummy phases into a descriptor.
 *
 * @param[in] op SPI memory operation to encode.
 * @param[in,out] desc Descriptor to populate.
 * @return 0 on success, DRIVER_ERROR_INVALID on an unsupported phase.
 */
static int sunxi_spif_encode_phases(const struct spi_mem_op *op, struct spif_descriptor_op *desc)
{
	int width;
	int addr_size;

	if (op->cmd.nbytes != 0U) {
		if (op->cmd.nbytes != 1U || op->cmd.opcode > 0xffU)
			return DRIVER_ERROR_INVALID;
		width = sunxi_spif_buswidth(op->cmd.buswidth);
		if (width < 0)
			return width;
		desc->trans_phase |= SPIF_TRANS_CMD_EN;
		desc->cmd_mode_buswidth |= (uint32_t)op->cmd.opcode << SPIF_CMD_OPCODE_POS;
		desc->cmd_mode_buswidth |= (uint32_t)width << sunxi_spif_platform_phase_pos(SUNXI_SPIF_PHASE_CMD);
		if (op->cmd.buswidth != SPIF_SINGLE_MODE)
			desc->cmd_mode_buswidth |= (uint32_t)width << SPIF_DATA_TRANS_POS;
	}

	if (op->addr.nbytes != 0U) {
		if (op->addr.val > 0xffffffffULL)
			return DRIVER_ERROR_INVALID;
		addr_size = sunxi_spif_platform_addr_size(op->addr.nbytes);
		if (addr_size < 0)
			return addr_size;
		width = sunxi_spif_buswidth(op->addr.buswidth);
		if (width < 0)
			return width;
		desc->trans_phase |= SPIF_TRANS_ADDR_EN;
		desc->flash_addr = (uint32_t)op->addr.val;
		desc->addr_dummy_data_count |= (uint32_t)addr_size;
		desc->cmd_mode_buswidth |= (uint32_t)width << sunxi_spif_platform_phase_pos(SUNXI_SPIF_PHASE_ADDR);
	}

	if (op->mode.val != NULL) {
		width = sunxi_spif_buswidth(op->mode.buswidth);
		if (width < 0)
			return width;
		desc->trans_phase |= SPIF_TRANS_MODE_EN;
		desc->cmd_mode_buswidth |= (uint32_t)*(const uint8_t *)op->mode.val << SPIF_MODE_OPCODE_POS;
		desc->cmd_mode_buswidth |= (uint32_t)width << sunxi_spif_platform_phase_pos(SUNXI_SPIF_PHASE_MODE);
	}

	if (op->dummy.nbytes != 0U) {
		desc->trans_phase |= SPIF_TRANS_DUMMY_EN;
		desc->addr_dummy_data_count |= (uint32_t)op->dummy.nbytes << SPIF_DUMMY_NUM_POS;
	}

	return 0;
}

/**
 * @brief Encode the data phase and resolve the DMA buffer address.
 *
 * Selects the transfer direction, allocates a bounce cache when the platform
 * requires one, and encodes the physical buffer address.
 *
 * @param[in] op SPI memory operation to encode.
 * @param[in,out] desc Descriptor to populate.
 * @param[in,out] buffers Bounce-buffer allocations for this operation.
 * @param[out] data_len Effective data length to transfer.
 * @param[out] buffer_addr Encoded DMA buffer address.
 * @param[out] bounce Set when a bounce buffer is used.
 * @param[out] bounce_rx Set when the bounce buffer holds received data.
 * @return 0 on success, DRIVER_ERROR_INVALID on failure.
 */
static int sunxi_spif_encode_data(const struct spi_mem_op *op, struct spif_descriptor_op *desc,
	struct sunxi_spif_op_buffers *buffers, u32 *data_len, u32 *buffer_addr, bool *bounce, bool *bounce_rx)
{
	uintptr_t addr;
	int ret;

	*data_len = op->data.nbytes;
	*bounce = false;
	*bounce_rx = false;
	*buffer_addr = 0U;
	if (op->data.nbytes == 0U)
		return op->data.dir == SPI_MEM_NO_DATA ? 0 : DRIVER_ERROR_INVALID;
	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (op->data.buf.in == NULL)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)op->data.buf.in;
		desc->trans_phase |= SPIF_TRANS_RX_EN;
		desc->hburst_rw_flag |= SPIF_DMA_RW_PROCESS;
		*bounce_rx = true;
	} else if (op->data.dir == SPI_MEM_DATA_OUT) {
		if (op->data.buf.out == NULL)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)op->data.buf.out;
		desc->trans_phase |= SPIF_TRANS_TX_EN;
		*bounce_rx = false;
	} else {
		return DRIVER_ERROR_INVALID;
	}

	if (sunxi_spif_platform_needs_short_read_bounce(op)) {
		if (sunxi_spif_alloc_aligned(
			    SPIF_MIN_TRANS_NUM, &buffers->cache_allocation, (void **)&buffers->cache) != DRIVER_OK)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)buffers->cache;
		*bounce = true;
		memset(buffers->cache, 0, SPIF_MIN_TRANS_NUM);
		*data_len = SPIF_MIN_TRANS_NUM;
	} else if (sunxi_spif_platform_needs_cache_bounce(addr)) {
		if (sunxi_spif_alloc_aligned(op->data.nbytes, &buffers->cache_allocation, (void **)&buffers->cache) !=
			DRIVER_OK)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)buffers->cache;
		*bounce = true;
		if (*bounce_rx)
			memset(buffers->cache, 0, op->data.nbytes);
		else
			memcpy(buffers->cache, op->data.buf.out, op->data.nbytes);
	}

	ret = sunxi_spif_platform_encode_data_addr(addr, buffer_addr);
	if (ret != 0)
		return ret;

	return 0;
}

/**
 * @brief Build the descriptor chain for a SPI memory operation.
 *
 * @param[in] op SPI memory operation to encode.
 * @param[in,out] buffers Descriptor and bounce-buffer allocations.
 * @param[out] descs First descriptor in the built chain.
 * @param[out] transfer_len Total data length to transfer.
 * @param[out] bounce Set when a bounce buffer is used.
 * @param[out] bounce_rx Set when the bounce buffer holds received data.
 * @return 0 on success, DRIVER_ERROR_INVALID on failure.
 */
static int sunxi_spif_build_descriptors(const struct spi_mem_op *op, struct sunxi_spif_op_buffers *buffers,
	struct spif_descriptor_op **descs, u32 *transfer_len, bool *bounce, bool *bounce_rx)
{
	struct spif_descriptor_op *first = buffers->desc;
	struct spif_descriptor_op *current;
	uint32_t max_transfer = sunxi_spif_platform_max_transfer();
	uint32_t data_len = op->data.nbytes;
	uint32_t encoded_data_addr;
	uint32_t descriptor_count;
	uint32_t descriptor_size;
	uint32_t offset;
	uint32_t index;
	int width;
	int ret;

	memset(first, 0, SUNXI_SPIF_DESC_SIZE);
	sunxi_spif_init_descriptor(first);
	ret = sunxi_spif_encode_phases(op, first);
	if (ret != 0)
		return ret;

	if (op->data.nbytes != 0U) {
		width = sunxi_spif_buswidth(op->data.buswidth);
		if (width < 0)
			return width;
		first->cmd_mode_buswidth |= (uint32_t)width << SPIF_DATA_TRANS_POS;
	}
	ret = sunxi_spif_encode_data(op, first, buffers, transfer_len, &encoded_data_addr, bounce, bounce_rx);
	if (ret != 0)
		return ret;
	first->data_addr = encoded_data_addr;
	data_len = *transfer_len;
	if (op->addr.nbytes != 0U && (uint64_t)first->flash_addr + data_len > 0x100000000ULL)
		return DRIVER_ERROR_INVALID;
	if (data_len > 0U) {
		descriptor_count = (data_len - 1U) / max_transfer + 1U;
		if (descriptor_count > SUNXI_SPIF_DESC_COUNT)
			return DRIVER_ERROR_INVALID;
	} else {
		descriptor_count = 1U;
	}
	descriptor_size = descriptor_count * sizeof(*first);
	if (descriptor_size > SUNXI_SPIF_DESC_SIZE)
		return DRIVER_ERROR_INVALID;

	if (descriptor_count > 1U)
		memset(first + 1, 0, (descriptor_count - 1U) * sizeof(*first));

	if (data_len == 0U) {
		first->hburst_rw_flag |= SPIF_DMA_FINISH_FLAG;
		*descs = first;
		return 0;
	}

	current = first;
	offset = 0U;
	for (index = 0U; index < descriptor_count; ++index) {
		uint32_t length = data_len - offset;
		uintptr_t descriptor_address;
		uintptr_t buffer_address;
		uint32_t next_address;

		if (length > max_transfer)
			length = max_transfer;
		if (index != 0U) {
			current = first + index;
			memcpy(current, first, sizeof(*current));
			current->next_des_addr = 0U;
			if (offset > 0xffffffffU - first->flash_addr)
				return DRIVER_ERROR_INVALID;
			current->flash_addr = first->flash_addr + offset;
			if (*bounce) {
				ret = sunxi_spif_add_offset((uintptr_t)buffers->cache, offset, &buffer_address);
			} else {
				buffer_address = (uintptr_t)(op->data.dir == SPI_MEM_DATA_IN ? op->data.buf.in :
											       op->data.buf.out);
				ret = sunxi_spif_add_offset(buffer_address, offset, &buffer_address);
			}
			if (ret == 0)
				ret = sunxi_spif_platform_encode_data_addr(buffer_address, &current->data_addr);
			if (ret != 0)
				return ret;
		}

		sunxi_spif_platform_set_data_length(&current->block_data_len, &current->addr_dummy_data_count, length);
		current->hburst_rw_flag &= ~SPIF_DMA_FINISH_FLAG;

		if (index + 1U < descriptor_count) {
			descriptor_address = (uintptr_t)(current + 1);
			ret = sunxi_spif_platform_encode_desc_addr(descriptor_address, &next_address);
			if (ret != 0)
				return ret;
			current->next_des_addr = next_address;
		}
		offset += length;
	}
	current->hburst_rw_flag |= SPIF_DMA_FINISH_FLAG;
	*descs = first;
	return 0;
}

/* Descriptor DMA transfer engine. */
/**
 * @brief Execute a descriptor-chain DMA transfer.
 *
 * Selects the DTR mode and clock, programs the general-control, phase, address,
 * and transfer registers for descriptor-based transfers, and polls for the DMA
 * done interrupt.  The cache is cleaned before and invalidated after the
 * transfer, and the mode and clock are restored on error.
 *
 * @param[in] spif SPI flash controller descriptor.
 * @param[in] desc First descriptor of the chain to execute.
 * @param[in] data_len Total data length to transfer in bytes.
 * @return 0 on success, -1 on failure.
 */
static int sunxi_spif_transfer(sunxi_spif_t *spif, struct spif_descriptor_op *desc, uint32_t data_len)
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
		if (!spif->dtr_active) {
			dtr_speed = (uint64_t)spif->speed_hz * 2U;
			if (dtr_speed > 0xffffffffULL || sunxi_spif_reconfigure_clock(spif, (uint32_t)dtr_speed) != 0) {
				ret = -1;
				goto restore_mode;
			}
			sunxi_spif_set_dtr_clock(spif, true);
			sunxi_spif_set_dtr(spif, true);
			spif->dtr_active = 1U;
			if (sunxi_spif_soft_reset(spif) != 0) {
				ret = -1;
				goto restore_mode;
			}
		}
	} else {
		if (spif->dtr_active) {
			sunxi_spif_set_dtr_clock(spif, false);
			sunxi_spif_set_dtr(spif, false);
			if (sunxi_spif_reconfigure_clock(spif, spif->speed_hz) != 0) {
				ret = -1;
				goto restore_mode;
			}
			spif->dtr_active = 0U;
		} else {
			sunxi_spif_set_dtr_clock(spif, false);
			sunxi_spif_set_dtr(spif, false);
		}
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
	if (ret != 0 && spif->dtr_active) {
		sunxi_spif_set_dtr_clock(spif, false);
		sunxi_spif_set_dtr(spif, false);
		if (sunxi_spif_reconfigure_clock(spif, spif->speed_hz) != 0)
			ret = -1;
		spif->dtr_active = 0U;
	}
	return ret;
}

/**
 * @brief Initialize the SPI flash controller.
 *
 * Validates the descriptor, applies default speed limits, initializes the GPIO
 * pins, enables the module clock, and programs the default bus frequency.
 *
 * @param[in,out] spif SPI flash controller descriptor.
 * @return 0 on success, -1 on invalid configuration or clock setup failure.
 */
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
	pr_info("initialized version=0x%x base=%p clock=%uHz\n", version, (void *)spif->base,
		spif->actual_speed_hz);
	return 0;
}

/**
 * @brief Disable the SPI flash controller.
 *
 * Clears the normal mode, disables the module clock, and marks the controller
 * as uninitialized.
 *
 * @param[in,out] spif SPI flash controller descriptor.
 */
void sunxi_spif_disable(sunxi_spif_t *spif)
{
	if (spif == NULL || spif->base == 0U)
		return;
	clrbits_le32(sunxi_spif_reg(spif, SPIF_GC_REG), SPIF_GC_NMODE_EN);
	sunxi_spif_clock_disable(spif);
	spif->initialized = 0U;
	spif->dtr_active = 0U;
}

/**
 * @brief Select the active SPI flash chip.
 *
 * @param[in,out] spif SPI flash controller descriptor.
 * @param[in] chip_select Chip-select number to activate.
 * @return 0 on success, -1 when the controller is invalid or the select fails.
 */
int sunxi_spif_select(sunxi_spif_t *spif, uint8_t chip_select)
{
	if (spif == NULL || spif->base == 0U || !spif->initialized)
		return -1;
	if (sunxi_spif_set_cs(spif, chip_select) != 0)
		return -1;
	spif->chip_select = chip_select;
	return 0;
}

/**
 * @brief Update the SPI flash bus clock.
 *
 * Validates the requested frequency against the minimum and the maximum
 * (doubled when DTR is active), disables DTR if it was active, and reprograms
 * the clock divider.
 *
 * @param[in,out] spif SPI flash controller descriptor.
 * @param[in] speed_hz Requested bus frequency in hertz.
 * @return 0 on success, -1 when out of range or the clock setup fails.
 */
int sunxi_spif_update_clk(sunxi_spif_t *spif, uint32_t speed_hz)
{
	uint32_t max_speed;

	if (spif == NULL || !spif->initialized || speed_hz == 0U)
		return -1;
	max_speed = spif->max_speed_hz * ((spif->rx_dtr_en || spif->tx_dtr_en) ? 2U : 1U);
	if (speed_hz < spif->min_speed_hz || speed_hz > max_speed)
		return -1;
	if (spif->dtr_active) {
		sunxi_spif_set_dtr_clock(spif, false);
		sunxi_spif_set_dtr(spif, false);
		spif->dtr_active = 0U;
	}
	if (sunxi_spif_reconfigure_clock(spif, speed_hz) != 0)
		return -1;
	spif->speed_hz = speed_hz;
	return 0;
}

/**
 * @brief Apply a set of configuration changes to the controller.
 *
 * Applies each flag-bearing field in the configuration: DTR enables, sample
 * mode and delay, and the bus frequency.  A sample-mode change is re-applied
 * when the controller is already initialized.
 *
 * @param[in,out] spif SPI flash controller descriptor.
 * @param[in] cfg Configuration fields to apply.
 * @return 0 on success, -1 on a null argument or invalid speed.
 */
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

/**
 * @brief Execute a SPI memory operation.
 *
 * Allocates the descriptor and bounce buffers, builds the descriptor chain for
 * the operation, runs the DMA transfer, and copies the bounce buffer back into
 * the operation data on a successful receive.
 *
 * @param[in,out] spif SPI flash controller descriptor.
 * @param[in] op SPI memory operation to execute.
 * @return 0 on success, a negative error code on failure.
 */
int sunxi_spif_exec_op(sunxi_spif_t *spif, const struct spi_mem_op *op)
{
	struct sunxi_spif_op_buffers buffers = { 0 };
	struct spif_descriptor_op *descs;
	u32 transfer_len;
	bool bounce;
	bool bounce_rx;
	int ret;

	if (spif == NULL || op == NULL || !spif->initialized)
		return DRIVER_ERROR_INVALID;
	ret = sunxi_spif_alloc_aligned(SUNXI_SPIF_DESC_SIZE, &buffers.desc_allocation, (void **)&buffers.desc);
	if (ret != DRIVER_OK)
		return ret;
	ret = sunxi_spif_build_descriptors(op, &buffers, &descs, &transfer_len, &bounce, &bounce_rx);
	if (ret != 0)
		goto out;

	ret = sunxi_spif_transfer(spif, descs, transfer_len);
	if (ret == 0 && bounce && bounce_rx && op->data.nbytes != 0U)
		memcpy(op->data.buf.in, buffers.cache, op->data.nbytes);

out:
	free(buffers.cache_allocation);
	free(buffers.desc_allocation);
	return ret;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-spif");
