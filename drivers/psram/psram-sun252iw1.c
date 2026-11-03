/* SPDX-License-Identifier: MIT */

#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <log.h>
#include <timer.h>
#include <drivers/psram/psram.h>
#include <driver.h>
#include <dt2c/driver.h>
#include <dt-compatible/psram-sqpi-dt.h>

#define SQPI_COMMON		 0x000U
#define SQPI_CONTROL		 0x004U
#define SQPI_CACHE		 0x008U
#define SQPI_COMMAND		 0x02cU
#define SQPI_ADDRESS		 0x030U
#define SQPI_DUMMY_HIGH		 0x034U
#define SQPI_DUMMY_LOW		 0x038U
#define SQPI_WAIT		 0x03cU
#define SQPI_TX_COUNT		 0x040U
#define SQPI_RX_COUNT		 0x044U
#define SQPI_START		 0x048U
#define SQPI_FIFO_LEVEL		 0x04cU
#define SQPI_FIFO_STATUS	 0x050U
#define SQPI_IRQ_ENABLE		 0x054U
#define SQPI_IRQ_STATUS		 0x058U
#define SQPI_MAP_START		 0x080U
#define SQPI_MAP_END		 0x084U
#define SQPI_MAP_BIAS		 0x088U
#define SQPI_RX_DATA		 0x200U
#define SQPI_PSRAM_COMMON	 0x800U
#define SQPI_PSRAM_CONTROL	 0x804U
#define SQPI_PSRAM_CACHE	 0x808U
#define SQPI_PSRAM_READ		 0x810U
#define SQPI_PSRAM_WRITE	 0x814U
#define SQPI_PSRAM_RD_DUMMY_HIGH 0x818U
#define SQPI_PSRAM_RD_DUMMY_LOW	 0x81cU
#define SQPI_PSRAM_WR_DUMMY_HIGH 0x820U
#define SQPI_PSRAM_WR_DUMMY_LOW	 0x824U
#define SQPI_PSRAM_WAIT		 0x828U
#define SQPI_PSRAM_SAMPLE	 0x86cU
#define SQPI_PSRAM_TIMING	 0x870U

#define SQPI_ADDRESS_24	       (2U << 30)
#define SQPI_IO_IDLE	       ((2U << 28) | (1U << 26) | (1U << 24))
#define SQPI_CACHE_CLEAR       (1U << 16)
#define SQPI_FIFO_RESET	       ((1U << 6) | (1U << 5))
#define SQPI_ARBITRATION       (1U << 4)
#define SQPI_MEMORY_ENABLE     (1U << 0)
#define SQPI_SELECT_PSRAM      (1U << 23)
#define SQPI_COMMAND_SINGLE    (1U << 20)
#define SQPI_COMMAND_QUAD      (3U << 20)
#define SQPI_ADDRESS_SINGLE    (1U << 16)
#define SQPI_ADDRESS_QUAD      (3U << 16)
#define SQPI_DUMMY_QUAD	       (3U << 12)
#define SQPI_DUMMY_BITS(count) ((count) << 4)
#define SQPI_DATA_SINGLE       1U
#define SQPI_DATA_QUAD	       3U
#define SQPI_CLOCK_SELECT      (1U << 17)
#define SQPI_FLASH_SAMPLE_HALF (1U << 12)
#define SQPI_FIFO_THRESHOLDS   0x6f0f6f0fU
#define SQPI_AUTO_FLUSH	       (1U << 31)
#define SQPI_READ_LINE_32      2U
#define SQPI_WRITE_LINE_64     (3U << 12)
#define SQPI_IRQ_ERRORS	       ((1U << 13) | (1U << 10))
#define SQPI_IRQ_CLEAR	       (SQPI_IRQ_ERRORS | (1U << 12))
#define SQPI_MAP_ENABLE	       (1U << 31)
#define SQPI_MAP_MASK	       0x3ffffff0U
#define SQPI_CLOCK_GATE	       (1U << 31)
#define SQPI_CLOCK_MASK	       ((7U << 24) | 31U | SQPI_CLOCK_GATE)
#define SQPI_TIMEOUT_US	       1000U
#define SQPI_AUTO_FLUSH_CYCLES 32U
#define SQPI_MAX_SIZE	       (16U * 1024U * 1024U)

#define PSRAM_EXIT_QPI	   0xf5U
#define PSRAM_RESET_ENABLE 0x66U
#define PSRAM_RESET	   0x99U
#define PSRAM_READ_ID	   0x9fU
#define PSRAM_QUAD_READ	   0xebU
#define PSRAM_QUAD_WRITE   0x38U

typedef struct {
	uintptr_t base;
	uintptr_t clock_reg;
	sunxi_clk_t clk;
	gpio_mux_t pins[6];
	uint32_t clock_source;
	uint32_t frequency;
	uint32_t actual_frequency;
	size_t expected_size;
	uint8_t id[3];
} sunxi_psram_sqpi_t;

static int sqpi_wait_clear(uintptr_t address, uint32_t mask)
{
	uint32_t start = time_us();

	while (readl(address) & mask) {
		if ((uint32_t)(time_us() - start) >= SQPI_TIMEOUT_US)
			return -1;
	}
	return 0;
}

static int sqpi_validate_config(const sunxi_psram_t *psram, const sunxi_psram_sqpi_t *sqpi)
{
	uint32_t divisor;

	if (!psram)
		return DRIVER_ERROR_INVALID;

	if (!sqpi->base || !sqpi->clock_reg || !sqpi->clk.gate_reg_base || !sqpi->clk.rst_reg_base ||
		sqpi->clk.gate_reg_offset >= 32U || sqpi->clk.rst_reg_offset >= 32U || sqpi->clock_source > 3U ||
		!sqpi->clk.parent_clk || !sqpi->frequency || sqpi->frequency > 104000000U || !psram->memory_size ||
		psram->memory_size > SQPI_MAX_SIZE || psram->memory_base != 0x12000000U || (psram->memory_size & 15U) ||
		sqpi->expected_size > psram->memory_size)
		return DRIVER_ERROR_INVALID;
	divisor = sqpi->clk.parent_clk / sqpi->frequency + (sqpi->clk.parent_clk % sqpi->frequency != 0U);
	if (!divisor || divisor > 32U)
		return DRIVER_ERROR_INVALID;
	for (size_t i = 0; i < 6U; i++) {
		if (!sqpi->pins[i].base || sqpi->pins[i].mux < 2U || sqpi->pins[i].mux >= GPIO_DISABLED)
			return DRIVER_ERROR_INVALID;
	}
	return DRIVER_OK;
}

static void sqpi_set_clock(sunxi_psram_sqpi_t *sqpi, uint32_t source, uint32_t parent, uint32_t frequency)
{
	uint32_t divisor = parent / frequency + (parent % frequency != 0U);
	uint32_t value = readl(sqpi->clock_reg) & ~SQPI_CLOCK_MASK;

	/* Stop the module clock while changing the source and divider. */
	writel(value, sqpi->clock_reg);
	writel(value | SQPI_CLOCK_GATE | (source << 24) | (divisor - 1U), sqpi->clock_reg);
	sqpi->actual_frequency = parent / divisor;
	udelay(2);
}

/* Only short, polled startup commands use SBUS; mapped traffic uses CBUS. */
static int sqpi_command(sunxi_psram_sqpi_t *sqpi, uint8_t opcode, uint32_t command_width, uint8_t *response)
{
	uint32_t command = ((uint32_t)opcode << 24) | SQPI_SELECT_PSRAM | command_width;
	uint32_t status;

	writel(readl(sqpi->base + SQPI_COMMON) | SQPI_FIFO_RESET, sqpi->base + SQPI_COMMON);
	if (sqpi_wait_clear(sqpi->base + SQPI_COMMON, SQPI_FIFO_RESET))
		return -1;
	writel(SQPI_IRQ_CLEAR, sqpi->base + SQPI_IRQ_STATUS);
	writel(0U, sqpi->base + SQPI_TX_COUNT);
	writel(response ? 3U : 0U, sqpi->base + SQPI_RX_COUNT);
	writel(0U, sqpi->base + SQPI_ADDRESS);
	if (response)
		command |= SQPI_ADDRESS_SINGLE | SQPI_DATA_SINGLE;
	writel(command, sqpi->base + SQPI_COMMAND);
	writel(1U, sqpi->base + SQPI_START);
	if (sqpi_wait_clear(sqpi->base + SQPI_START, 1U)) {
		pr_err("SQPI command %02x timed out\n", opcode);
		return -1;
	}
	/* The PSRAM CBUS timeout indication is independent of SBUS commands. */
	status = readl(sqpi->base + SQPI_IRQ_STATUS);
	if (status & SQPI_IRQ_ERRORS) {
		pr_err("SQPI command %02x status %08x\n", opcode, status);
		return -1;
	}
	if (response) {
		if ((readl(sqpi->base + SQPI_FIFO_STATUS) & 0xffU) != 3U)
			return -1;
		for (size_t i = 0; i < 3U; i++)
			response[i] = readb(sqpi->base + SQPI_RX_DATA);
	}
	writel(SQPI_IRQ_CLEAR, sqpi->base + SQPI_IRQ_STATUS);
	return 0;
}

static size_t sqpi_detect_size(const uint8_t id[3])
{
	uint32_t density = id[2] >> 5;

	/* 0x5d is the known-good-die marker; density 0/1/2 is 16/32/64 Mbit. */
	if (!id[0] || id[0] == 0xffU || id[1] != 0x5dU || density > 2U)
		return 0U;
	return (2U * 1024U * 1024U) << density;
}

static void sqpi_disable(sunxi_psram_t *psram, const sunxi_psram_sqpi_t *sqpi)
{
	writel(readl(sqpi->base + SQPI_COMMON) & ~SQPI_MEMORY_ENABLE, sqpi->base + SQPI_COMMON);
	for (size_t i = 0; i < 8U; i++)
		writel(0U, sqpi->base + SQPI_MAP_BIAS + i * 16U);
	psram->size = 0U;
}

static void sqpi_init_pins(const sunxi_psram_sqpi_t *sqpi)
{
	for (size_t i = 0; i < 6U; i++) {
		sunxi_gpio_set_pull(&sqpi->pins[i], i == 0U ? GPIO_PULL_UP : GPIO_PULL_NONE);
		sunxi_gpio_set_drv(&sqpi->pins[i], 1U);
		sunxi_gpio_init(&sqpi->pins[i]);
	}
}

static void sqpi_enable_bus(sunxi_psram_sqpi_t *sqpi)
{
	writel(readl(sqpi->clk.rst_reg_base) & ~(1U << sqpi->clk.rst_reg_offset), sqpi->clk.rst_reg_base);
	writel(readl(sqpi->clk.gate_reg_base) | (1U << sqpi->clk.gate_reg_offset), sqpi->clk.gate_reg_base);
	sqpi_set_clock(sqpi, 0U, 24000000U, 24000000U);
	writel(readl(sqpi->clk.rst_reg_base) | (1U << sqpi->clk.rst_reg_offset), sqpi->clk.rst_reg_base);
	udelay(200);
}

static void sqpi_prepare_controller(sunxi_psram_t *psram, const sunxi_psram_sqpi_t *sqpi)
{
	writel(SQPI_ADDRESS_24 | SQPI_IO_IDLE | SQPI_ARBITRATION, sqpi->base + SQPI_COMMON);
	/* Mode 0, CS1 for PSRAM, one half-cycle flash sample delay. */
	writel(SQPI_CLOCK_SELECT | SQPI_FLASH_SAMPLE_HALF, sqpi->base + SQPI_CONTROL);
	writel(0U, sqpi->base + SQPI_IRQ_ENABLE);
	writel(0U, sqpi->base + SQPI_WAIT);
	writel(0U, sqpi->base + SQPI_DUMMY_HIGH);
	writel(0U, sqpi->base + SQPI_DUMMY_LOW);
	writel(SQPI_FIFO_THRESHOLDS, sqpi->base + SQPI_FIFO_LEVEL);
	writel(1U, sqpi->base + SQPI_PSRAM_SAMPLE);
	sqpi_disable(psram, sqpi);
}

static int sqpi_reset_chip(sunxi_psram_sqpi_t *sqpi)
{
	/* Recover a device left in QPI by an earlier SRAM application. */
	if (sqpi_command(sqpi, PSRAM_EXIT_QPI, SQPI_COMMAND_QUAD, NULL) ||
		sqpi_command(sqpi, PSRAM_RESET_ENABLE, SQPI_COMMAND_SINGLE, NULL) ||
		sqpi_command(sqpi, PSRAM_RESET, SQPI_COMMAND_SINGLE, NULL))
		return -1;
	udelay(200);
	return 0;
}

static size_t sqpi_probe_chip(const sunxi_psram_t *psram, sunxi_psram_sqpi_t *sqpi)
{
	size_t detected_size;

	if (sqpi_reset_chip(sqpi) || sqpi_command(sqpi, PSRAM_READ_ID, SQPI_COMMAND_SINGLE, sqpi->id))
		return 0U;
	pr_info("PSRAM ID: %02x %02x %02x\n", sqpi->id[0], sqpi->id[1], sqpi->id[2]);
	detected_size = sqpi_detect_size(sqpi->id);
	if (!detected_size || detected_size > psram->memory_size ||
		(sqpi->expected_size && detected_size != sqpi->expected_size)) {
		pr_err("PSRAM unsupported ID or capacity mismatch\n");
		return 0U;
	}
	return detected_size;
}

static void sqpi_configure_cache(const sunxi_psram_sqpi_t *sqpi)
{
	writel(SQPI_ADDRESS_24, sqpi->base + SQPI_PSRAM_COMMON);
	writel(0U, sqpi->base + SQPI_PSRAM_CONTROL);
	/* Flush partial writes after 32 clocks; finish each burst within a page. */
	writel(SQPI_AUTO_FLUSH | (SQPI_AUTO_FLUSH_CYCLES << 16), sqpi->base + SQPI_PSRAM_CACHE);
	/* Keep the reset page/CE timing; cache lines are smaller than a page. */
	writel(SQPI_READ_LINE_32 | SQPI_WRITE_LINE_64, sqpi->base + SQPI_CACHE);
}

static void sqpi_configure_protocol(const sunxi_psram_sqpi_t *sqpi)
{
	/* 1-4-4 SPI, six dummy clocks for 0xeb, no QPI state to maintain. */
	writel((PSRAM_QUAD_READ << 24) | SQPI_COMMAND_SINGLE | SQPI_ADDRESS_QUAD | SQPI_DUMMY_QUAD |
			SQPI_DUMMY_BITS(24U) | SQPI_DATA_QUAD,
		sqpi->base + SQPI_PSRAM_READ);
	writel((PSRAM_QUAD_WRITE << 24) | SQPI_COMMAND_SINGLE | SQPI_ADDRESS_QUAD | SQPI_DATA_QUAD,
		sqpi->base + SQPI_PSRAM_WRITE);
	writel(0U, sqpi->base + SQPI_PSRAM_WAIT);
	writel(0U, sqpi->base + SQPI_PSRAM_RD_DUMMY_HIGH);
	writel(0U, sqpi->base + SQPI_PSRAM_RD_DUMMY_LOW);
	writel(0U, sqpi->base + SQPI_PSRAM_WR_DUMMY_HIGH);
	writel(0U, sqpi->base + SQPI_PSRAM_WR_DUMMY_LOW);
	writel(sqpi->actual_frequency > 80000000U ? 2U : 1U, sqpi->base + SQPI_PSRAM_SAMPLE);
}

static int sqpi_clear_read_cache(const sunxi_psram_sqpi_t *sqpi)
{
	writel(readl(sqpi->base + SQPI_COMMON) | SQPI_CACHE_CLEAR, sqpi->base + SQPI_COMMON);
	return sqpi_wait_clear(sqpi->base + SQPI_COMMON, SQPI_CACHE_CLEAR);
}

static int sqpi_map_memory(const sunxi_psram_t *psram, const sunxi_psram_sqpi_t *sqpi, size_t capacity)
{
	writel((uint32_t)psram->memory_base & SQPI_MAP_MASK, sqpi->base + SQPI_MAP_START);
	writel((uint32_t)(psram->memory_base + capacity - 1U) & SQPI_MAP_MASK, sqpi->base + SQPI_MAP_END);
	writel(SQPI_MAP_ENABLE, sqpi->base + SQPI_MAP_BIAS);
	if (sqpi_clear_read_cache(sqpi))
		return -1;
	writel(readl(sqpi->base + SQPI_COMMON) | SQPI_MEMORY_ENABLE, sqpi->base + SQPI_COMMON);
	return 0;
}

static int sqpi_read_resources(const sunxi_psram_t *psram, sunxi_psram_sqpi_t *sqpi)
{
	*sqpi = (sunxi_psram_sqpi_t){ 0 };
	if (sunxi_sqpi_psram_dt_read_clock(psram->dt_node, &sqpi->clock_reg, &sqpi->clock_source, &sqpi->clk) ||
		sunxi_sqpi_psram_dt_read_pins(psram->dt_node, sqpi->pins) ||
		sunxi_sqpi_psram_dt_read_settings(psram->dt_node, &sqpi->base, &sqpi->frequency, &sqpi->expected_size))
		return DRIVER_ERROR_INVALID;
	return sqpi_validate_config(psram, sqpi);
}

uint32_t sunxi_psram_init(sunxi_psram_t *psram)
{
	sunxi_psram_sqpi_t sqpi;
	size_t capacity;

	if (!psram)
		return 0U;
	/* Reinitializing a live memory controller would destroy application data. */
	if (psram->size)
		return psram->size;
	if (sqpi_read_resources(psram, &sqpi) != DRIVER_OK) {
		pr_err("SQPI invalid resources or frequency\n");
		return 0U;
	}
	sqpi_init_pins(&sqpi);
	sqpi_enable_bus(&sqpi);
	sqpi_prepare_controller(psram, &sqpi);
	capacity = sqpi_probe_chip(psram, &sqpi);
	if (!capacity)
		goto fail;
	sqpi_set_clock(&sqpi, sqpi.clock_source, sqpi.clk.parent_clk, sqpi.frequency);
	sqpi_configure_cache(&sqpi);
	sqpi_configure_protocol(&sqpi);
	if (sqpi_map_memory(psram, &sqpi, capacity))
		goto fail;
	psram->memory_size = capacity;
	psram->size = capacity / (1024U * 1024U);
	pr_info("PSRAM: %u MiB at %08x, SQPI %u Hz\n", psram->size, (uint32_t)psram->memory_base,
		sqpi.actual_frequency);
	return psram->size;

fail:
	sqpi_disable(psram, &sqpi);
	return 0U;
}

DT2C_DRIVER_COMPAT("allwinner,sun252iw1-sqpi-psram");
