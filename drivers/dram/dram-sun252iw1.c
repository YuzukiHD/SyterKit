/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "dram-sun252iw1: " fmt

/**
 * @file dram-sun252iw1.c
 * @brief DRAM controller driver for the Allwinner sun252iw1 SoC.
 *
 * Provides the microsecond delay, cache maintenance, DDR voltage and DDR IO
 * resistor calibration hooks used by the DRAM init blob, and drives the
 * external DRAM initialization routine.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <dt2c/driver.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/pmu/axp.h>

#include <common.h>

/* -------------------------------------------------------------------------
 * Register map
 *
 * The DDR IO resistor calibration is performed by sampling the resistor
 * divider output through GPADC channel 4 and steering the result back into
 * the RESCAL block.  All bases/offsets come from the sun252iw1p1 reference
 * (res_test.h / clock_autogen.h).
 * ---------------------------------------------------------------------- */

#define SUN252IW1_CCU_BASE    0x02001000U
#define SUN252IW1_GPADC_BASE  0x02009000U
#define SUN252IW1_THS_BASE    0x02009400U
#define SUN252IW1_RESCAL_BASE 0x03000000U

/* CCU registers. */
#define SUN252IW1_CCU_CLK24M_GATE_EN (SUN252IW1_CCU_BASE + 0xE0CU)
#define SUN252IW1_CCU_ADC_CLK_SEL    (SUN252IW1_CCU_BASE + 0xF04U)
#define SUN252IW1_CCU_ADC_BGR	     (SUN252IW1_CCU_BASE + 0x9ECU)
#define SUN252IW1_CCU_THS_BGR	     (SUN252IW1_CCU_BASE + 0x9FCU)

/* THS / GPADC LDO register. */
#define SUN252IW1_GPADC_LDO_EN (SUN252IW1_THS_BASE + 0x04U)

/* GPADC registers. */
#define SUN252IW1_GP_SR_CON	(SUN252IW1_GPADC_BASE + 0x00U)
#define SUN252IW1_GP_CTRL	(SUN252IW1_GPADC_BASE + 0x04U)
#define SUN252IW1_GP_CS_EN	(SUN252IW1_GPADC_BASE + 0x08U)
#define SUN252IW1_GP_DATAL_INTC (SUN252IW1_GPADC_BASE + 0x28U)
#define SUN252IW1_GP_DATAL_INTS (SUN252IW1_GPADC_BASE + 0x38U)
#define SUN252IW1_GP_CH0_DATA	(SUN252IW1_GPADC_BASE + 0x80U)

/* RESCAL (resistor calibration) registers. */
#define SUN252IW1_RESCAL_CTRL  (SUN252IW1_RESCAL_BASE + 0x160U)
#define SUN252IW1_DDR_RES_CTRL (SUN252IW1_RESCAL_BASE + 0x164U)

/* GPADC channel and resistor calibration modes. */
#define SUN252IW1_GPADC_DDR_CHANNEL 4U
#define SUN252IW1_RES_MODE_INTERNAL 1U /* PHY internal calibration. */
#define SUN252IW1_RES_MODE_EXTERNAL 3U /* External DDR calibration. */

extern int init_DRAM(int type, void *buff);

/**
 * @brief Delay for a number of microseconds.
 *
 * @param[in] us Delay duration in microseconds.
 */
void __usdelay(unsigned long us)
{
	udelay(us);
}

/**
 * @brief Invalidate and clear the L2 cache.
 *
 * Invalidates the entire data cache so stale entries do not survive the DRAM
 * initialization.
 */
void csi_l2c_clear_invalid_all(void)
{
	invalidate_dcache_all();
	return;
}

/**
 * @brief Flush the L2 cache.
 *
 * Flushes the entire data cache so pending writes are visible after the DRAM
 * initialization.
 */
void csi_l2c_clear_all(void)
{
	flush_dcache_all();
	return;
}

/**
 * @brief Set the DDR supply voltage.
 *
 * The voltage is configured by the external DRAM init blob, so this is a
 * no-op on this SoC.
 *
 * @param[in] vol_val Target voltage value in millivolts.
 *
 * @return 0 on success.
 */
int set_ddr_voltage(unsigned int vol_val)
{
	return 0;
}

/* -------------------------------------------------------------------------
 * DDR IO resistor calibration
 *
 * The precompiled DRAM init blob calls res_test() from
 * mctl_phy_zq_calibration() and combines the returned code with the write
 * enable pattern (0x19180000) for the DDR_RES_CTRL register, so this
 * implementation must stay resident and return the last calibrated code.
 * ---------------------------------------------------------------------- */

/**
 * @brief Enable the GPADC clock.
 *
 * Selects the 24 MHz HOSC clock for the GPADC, releases it from reset and
 * enables its clock gating.
 *
 * @return 0 on success.
 */
static int gpadc_clock_init(void)
{
	uint32_t reg_val;

	/* switch the clock source to 24M */
	reg_val = readl(SUN252IW1_CCU_ADC_CLK_SEL);
	reg_val &= ~(0x5U << 20);
	writel(reg_val, SUN252IW1_CCU_ADC_CLK_SEL);

	reg_val = readl(SUN252IW1_CCU_ADC_CLK_SEL);
	reg_val |= (0x5U << 20);
	writel(reg_val, SUN252IW1_CCU_ADC_CLK_SEL);

	/* reset */
	reg_val = readl(SUN252IW1_CCU_ADC_BGR);
	reg_val &= ~(1U << 16);
	writel(reg_val, SUN252IW1_CCU_ADC_BGR);

	udelay(2);

	reg_val |= (1U << 16);
	writel(reg_val, SUN252IW1_CCU_ADC_BGR);

	/* enable GPADC clock gating */
	reg_val = readl(SUN252IW1_CCU_ADC_BGR);
	reg_val |= (1U << 0);
	writel(reg_val, SUN252IW1_CCU_ADC_BGR);

	return 0;
}

/**
 * @brief Configure the GPADC for a single-ended channel acquisition.
 *
 * Enables the 24M clock gate, the GPADC/THS clock domains and the GPADC LDO,
 * then programs the sample rate divider, the auto-calibration mode and the
 * data-low interrupt of the selected channel.
 *
 * @param[in] channel GPADC channel number to enable.
 *
 * @return 0 on success.
 */
static int gpadc_simple_config_init(uint32_t channel)
{
	uint32_t reg_val;

	/* enable the 24M clock gate for the GPADC */
	reg_val = readl(SUN252IW1_CCU_CLK24M_GATE_EN);
	reg_val |= (1U << 2);
	writel(reg_val, SUN252IW1_CCU_CLK24M_GATE_EN);

	/* select HOSC/1 as the GPADC clock source */
	writel(0x5U << 20, SUN252IW1_CCU_ADC_CLK_SEL);

	/* deassert reset and enable the GPADC clock */
	reg_val = readl(SUN252IW1_CCU_ADC_BGR);
	reg_val |= (1U << 16);
	reg_val |= (1U << 0);
	writel(reg_val, SUN252IW1_CCU_ADC_BGR);

	/* deassert reset and enable the THS clock */
	reg_val = readl(SUN252IW1_CCU_THS_BGR);
	reg_val |= (1U << 16);
	reg_val |= (1U << 0);
	writel(reg_val, SUN252IW1_CCU_THS_BGR);

	/* enable the GPADC LDO */
	writel(1U << 16, SUN252IW1_GPADC_LDO_EN);
	udelay(50);

	/* ADC acquire time */
	reg_val = readl(SUN252IW1_GP_SR_CON);
	reg_val &= ~(0xFFFFU << 0);
	reg_val |= (0x164U << 0);
	writel(reg_val, SUN252IW1_GP_SR_CON);

	/* ADC sample frequency divider */
	reg_val = readl(SUN252IW1_GP_SR_CON);
	reg_val &= ~(0xFFFFU << 16);
	reg_val |= (0x1DFU << 16);
	writel(reg_val, SUN252IW1_GP_SR_CON);

	/* work mode 2 with auto calibration enabled */
	reg_val = readl(SUN252IW1_GP_CTRL);
	reg_val &= ~(0x3U << 18);
	reg_val |= (0x2U << 18);
	reg_val |= (0x1U << 23); /* ADC AUTOCAL_EN */
	writel(reg_val, SUN252IW1_GP_CTRL);

	/* enable the ADC function */
	reg_val = readl(SUN252IW1_GP_CTRL);
	reg_val &= ~(0x1U << 16);
	reg_val |= (0x1U << 16);
	writel(reg_val, SUN252IW1_GP_CTRL);

	/* enable the analog input channel */
	reg_val = readl(SUN252IW1_GP_CS_EN);
	reg_val &= ~(0x1U << channel);
	reg_val |= (0x1U << channel);
	writel(reg_val, SUN252IW1_GP_CS_EN);
	udelay(50);

	/* enable the channel data-low available interrupt */
	reg_val = readl(SUN252IW1_GP_DATAL_INTC);
	reg_val &= ~(0x1U << channel);
	reg_val |= (0x1U << channel);
	writel(reg_val, SUN252IW1_GP_DATAL_INTC);

	return 0;
}

/**
 * @brief Average a number of conversions from a GPADC channel.
 *
 * Starts a conversion by raising the data-low interrupt, waits for it to
 * complete, then accumulates the 12-bit result.
 *
 * @param[in] channel GPADC channel number to sample.
 *
 * @return Averaged raw ADC value.
 */
static uint32_t gpadc_read_channel(uint32_t channel)
{
	const uint32_t loop = 10U;
	uint32_t rdata = 0U;
	uint32_t cnt;

	for (cnt = 0; cnt < loop; cnt++) {
		/* start the conversion */
		writel(0x1U << channel, SUN252IW1_GP_DATAL_INTS);
		while (!(readl(SUN252IW1_GP_DATAL_INTS) & (1U << channel)))
			; /* wait for the data to become available */
		rdata += readl(SUN252IW1_GP_CH0_DATA + channel * 4U);
	}

	return rdata / loop;
}

/**
 * @brief Select and enable a resistor calibration source.
 *
 * @param[in] source Calibration source: 0/1 selects the internal (PHY)
 *                   resistor, 3 selects the external (DDR) resistor.
 *
 * @return The selected source.
 */
static uint32_t rescali_select_source(uint32_t source)
{
	uint32_t reg_val;

	/* disable resistor calibration first */
	writel(0, SUN252IW1_RESCAL_CTRL);

	/* select the calibration source */
	reg_val = readl(SUN252IW1_RESCAL_CTRL);
	reg_val &= ~(0x3U << 2);
	reg_val |= (source << 2);
	writel(reg_val, SUN252IW1_RESCAL_CTRL);

	/* enable resistor calibration */
	reg_val = readl(SUN252IW1_RESCAL_CTRL);
	reg_val &= ~(0x1U << 1);
	reg_val |= (0x1U << 1);
	writel(reg_val, SUN252IW1_RESCAL_CTRL);

	return source;
}

/**
 * @brief Map a sampled ADC value to a resistor calibration code.
 *
 * The ADC output voltage divides the reference range into 16 bands of 21
 * counts starting at 1870.  The PHY-internal modes (0/1) use the identity
 * mapping while the external DDR mode (3) uses a board-specific table.
 *
 * @param[in] adc_value Sampled GPADC raw value.
 * @param[in] mode      Resistor calibration mode.
 *
 * @return Resistor calibration code.
 */
static uint32_t rescali_calibrate(uint32_t adc_value, uint32_t mode)
{
	static const uint32_t code_internal[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	static const uint32_t code_external[16] = { 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 12, 13 };
	const uint32_t *table = (mode == SUN252IW1_RES_MODE_EXTERNAL) ? code_external : code_internal;
	const uint32_t step = 21U;
	uint32_t range_min = 1870U;
	uint32_t code = 0U;
	uint32_t i;

	for (i = 0; i < 16U; i++) {
		if (adc_value >= range_min && adc_value <= range_min + step - 1U) {
			code = table[i];
			break;
		}
		range_min += step;
	}

	pr_debug("MODE %d CODE_B = 0x%x\n", mode, code);

	return code;
}

/**
 * @brief Write the calibrated code to the DDR RES control register.
 *
 * Only the external DDR mode (3) programs the register; the value is merged
 * with the 0x19180000 write-enable pattern.
 *
 * @param[in] code Calibrated resistor code.
 * @param[in] mode Resistor calibration mode.
 */
static void rescali_write_result(uint32_t code, uint32_t mode)
{
	if (mode == SUN252IW1_RES_MODE_EXTERNAL)
		writel(0x19180000U | code, SUN252IW1_DDR_RES_CTRL);
}

/**
 * @brief Run the GPADC-based DDR IO resistor calibration.
 *
 * Calibrates both the PHY-internal (mode 1) and the external DDR (mode 3)
 * resistors, keeping the last calibrated code as the result.  The GPADC and
 * the RESCAL block are switched off again before returning.
 *
 * @return The resistor calibration code for the external DDR mode.
 */
static uint32_t rescali_gpadc_test(void)
{
	const uint32_t channel = SUN252IW1_GPADC_DDR_CHANNEL;
	uint32_t code = 0U;
	uint32_t mode;
	uint32_t adc_value;

	gpadc_clock_init();
	gpadc_simple_config_init(channel);

	for (mode = SUN252IW1_RES_MODE_INTERNAL; mode <= SUN252IW1_RES_MODE_EXTERNAL; mode++) {
		if (mode == 2U)
			mode++;

		rescali_select_source(mode);
		adc_value = gpadc_read_channel(channel);
		pr_debug("GPADC DATA = %d\n", adc_value);

		code = rescali_calibrate(adc_value, mode);
		rescali_write_result(code, mode);
	}

	/* disable the GPADC clock and resistor calibration */
	writel(0, SUN252IW1_CCU_ADC_BGR);
	writel(0, SUN252IW1_RESCAL_CTRL);

	return code;
}

/**
 * @brief DDR IO resistor calibration entry point.
 *
 * Invoked by the external DRAM init blob during the PHY ZQ calibration
 * step.  The returned code is combined with the write-enable pattern and
 * stored in the DDR RES control register by the caller.
 *
 * @return The last calibrated resistor code (external DDR mode).
 */
uint32_t res_test(void)
{
	return rescali_gpadc_test();
}

/**
 * @brief Initialize the DRAM controller.
 *
 * Validates the DRAM parameters and delegates to the external init_DRAM
 * routine.
 *
 * @param[in] dram DRAM configuration block.
 *
 * @return Detected DRAM size in bytes, or 0 on failure.
 */
uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
