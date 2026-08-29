/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file
 * @brief System PWM (Pulse Width Modulation) driver for Allwinner (sunxi) platforms
 * @details This file implements PWM functionality for Allwinner SoCs, supporting
 *          multiple channels with configurable frequency, duty cycle, polarity,
 *          and dead zone timing. It provides both single-channel and dual-channel
 *          binding modes, with support for pulse mode operations.
 *          The driver handles various clock sources including APB and oscillator
 *          clocks, and provides fine-grained control over PWM output parameters.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/pwm/pwm.h>
#include <dt2c/driver.h>

#define PRESCALE_MAX ((1U << PWM_PRESCAL_WIDTH) - 1U)

/**
 * @brief Pre-scaler values for clock division.
 *
 * This constant array maps register values (`reg_val`) to corresponding clock pre-divider values (`clk_pre_div`).
 * The array is used to configure the pre-scaler for clock division in PWM or similar hardware modules.
 *
 * The first element in each pair is the register value, and the second element is the associated clock division factor.
 */
static const uint32_t pre_scal[][2] = {
	/* reg_val clk_pre_div */
	{ 0, 1 }, { 1, 2 }, { 2, 4 }, { 3, 8 }, { 4, 16 }, { 5, 32 }, { 6, 64 }, { 7, 128 }, { 8, 256 },
};

/**
 * @brief Perform division with rounding for PWM calculations.
 *
 * This macro performs a division operation on a given number `n` by `base`, and applies rounding
 * to the result. The division is done by casting both operands to `uint64_t` to avoid overflow
 * for large values. If the remainder is greater than half of `base`, the quotient is incremented
 * by 1, implementing rounding to the nearest integer.
 *
 * @param n       The numerator, which will be updated with the quotient.
 * @param base    The divisor.
 *
 * @return The remainder of the division.
 */
#define sunxi_pwm_do_div(n, base)                 \
	do {                                      \
		uint32_t __base = (base);         \
		uint32_t __rem;                   \
		__rem = ((uint64_t)(n)) % __base; \
		(n) = ((uint64_t)(n)) / __base;   \
		if (__rem > __base / 2)           \
			++(n);                    \
	} while (0)

static int sunxi_pwm_validate_channel(const sunxi_pwm_t *pwm, int channel)
{
	if (pwm == NULL)
		return -1;

	if (channel < 0 || channel >= (int)SUNXI_PWM_CHANNEL_MAX || !(pwm->channel_mask & BIT(channel)))
		return -1;

	if (pwm->channel[channel].channel_mode != PWM_CHANNEL_SINGLE && pwm->channel[channel].channel_mode != PWM_CHANNEL_BIND)
		return -1;

	return 0;
}

static int sunxi_pwm_validate_config(const sunxi_pwm_config_t *config)
{
	if (config == NULL)
		return -1;

	if (config->period_ns == 0U || config->duty_ns > config->period_ns)
		return -1;

	if ((int)config->polarity < PWM_POLARITY_INVERSED || (int)config->polarity > PWM_POLARITY_NORMAL)
		return -1;

	if ((int)config->pwm_mode < PWM_MODE_CYCLE || (int)config->pwm_mode > PWM_MODE_PLUSE)
		return -1;

	if (config->pwm_mode == PWM_MODE_PLUSE && (config->pluse_count == 0U || config->pluse_count >= (1U << PWM_PULSE_NUM_WIDTH)))
		return -1;

	return 0;
}

/**
 * @brief Set the value of a specific field in a PWM register.
 *
 * This function modifies a specific field in the PWM register by shifting and masking the data.
 * It reads the current register value, applies the mask and shift to set the field value, and writes
 * the modified value back to the register.
 *
 * @param reg         The base address of the register.
 * @param reg_shift   The bit position to shift the data for the specific field.
 * @param reg_width   The width (size) of the field to be modified (in bits).
 * @param data        The data to be written to the register field.
 */
static inline void sunxi_pwm_reg_set(uint32_t reg, uint32_t reg_shift, uint32_t reg_width, uint32_t data)
{
	if (reg_shift >= 32U || reg_width == 0U || reg_width > 32U || reg_shift > 32U - reg_width)
		return;

	uint32_t field_mask = reg_width >= 32U ? 0xffffffffU : (reg_width ? ((1U << reg_width) - 1U) : 0U);

	data &= field_mask;
	writel((readl(reg) & ~(field_mask << reg_shift)) | (data << reg_shift), reg);
}

/**
 * @brief Initialize the GPIO for a specific PWM channel.
 *
 * This function initializes the GPIO pins associated with a given PWM channel. If the channel is bound
 * to another channel (via a binding mode), the GPIO for the bound channel is also initialized. The
 * initialization process involves configuring the pin and its multiplexing function as defined in the
 * PWM controller structure.
 *
 * @param pwm Pointer to the PWM controller structure.
 * @param channel The PWM channel number (0-15) whose GPIO needs to be initialized.
 *
 * @note If the specified channel is in the "PWM_CHANNEL_BIND" mode, the GPIO for the bound channel will
 *       also be initialized first before initializing the specified channel's GPIO.
 */
static inline void sunxi_pwm_gpio_init(sunxi_pwm_t *pwm, int channel)
{
	if (sunxi_pwm_validate_channel(pwm, channel))
		return;

	if (pwm->channel[channel].channel_mode == PWM_CHANNEL_BIND) {
		uint32_t bind_channel = pwm->channel[channel].bind_channel;
		if (bind_channel >= SUNXI_PWM_CHANNEL_MAX || bind_channel == (uint32_t)channel || !(pwm->channel_mask & BIT(bind_channel)))
			return;

		sunxi_gpio_init(&pwm->channel[bind_channel].pin);
	}
	sunxi_gpio_init(&pwm->channel[channel].pin);
}

/**
 * @brief Initialize the PWM clock by configuring the bus and peripheral clocks.
 *
 * This function initializes the PWM clock by configuring its reset and gate controls.
 *
 * @param pwm Pointer to the PWM controller structure.
 *
 * @note A short delay allows the gate state to stabilize before it is enabled.
 */
static inline void sunxi_pwm_clk_init(sunxi_pwm_t *pwm)
{
	/* if using clk */
	if (pwm == NULL)
		return;
	/* Match SPL hal_clk_disable/enable: close gate, assert reset,
	 * then release reset before reopening the gate. */
	clrbits_le32(pwm->pwm_clk.gate_reg_base, BIT(pwm->pwm_clk.gate_reg_offset));
	clrbits_le32(pwm->pwm_clk.rst_reg_base, BIT(pwm->pwm_clk.rst_reg_offset));
	udelay(10);
	setbits_le32(pwm->pwm_clk.rst_reg_base, BIT(pwm->pwm_clk.rst_reg_offset));
	setbits_le32(pwm->pwm_clk.gate_reg_base, BIT(pwm->pwm_clk.gate_reg_offset));
}

/**
 * @brief Deinitialize the PWM clock and gate/reset control.
 *
 * This function disables the clock and resets the PWM controller by clearing the appropriate
 * bits in the clock gate and reset registers.
 *
 * @param pwm Pointer to the PWM controller structure.
 *
 * @note This function will only affect the clock if the corresponding clock base addresses
 *       are properly initialized in the PWM structure.
 */
static inline void sunxi_pwm_clk_deinit(sunxi_pwm_t *pwm)
{
	/* if using clk */
	if (pwm == NULL)
		return;
	/* Close Gate */
	clrbits_le32(pwm->pwm_clk.gate_reg_base, BIT(pwm->pwm_clk.gate_reg_offset));
	/* Assert CLK RST */
	clrbits_le32(pwm->pwm_clk.rst_reg_base, BIT(pwm->pwm_clk.rst_reg_offset));
}

/**
 * @brief Enable the PWM controller for a specific channel.
 *
 * This function enables the PWM controller for the given channel by setting the corresponding bit
 * in the PWM controller's "PWM_PER" register.
 *
 * @param pwm Pointer to the PWM controller structure.
 * @param channel The PWM channel number (0-15) to enable.
 */
static inline void sunxi_pwm_enable_controller(sunxi_pwm_t *pwm, int channel)
{
	if (sunxi_pwm_validate_channel(pwm, channel))
		return;
	setbits_le32(pwm->base + PWM_PER, BIT(channel));
}

/**
 * @brief Disable the PWM controller for a specific channel.
 *
 * This function disables the PWM controller for the given channel by clearing the corresponding bit
 * in the PWM controller's "PWM_PER" register.
 *
 * @param pwm Pointer to the PWM controller structure.
 * @param channel The PWM channel number (0-15) to disable.
 */
static inline void sunxi_pwm_disable_controller(sunxi_pwm_t *pwm, int channel)
{
	if (sunxi_pwm_validate_channel(pwm, channel))
		return;
	clrbits_le32(pwm->base + PWM_PER, BIT(channel));
}

/**
 * @brief Set the polarity of a specific PWM channel.
 *
 * This function sets the polarity for the specified PWM channel. The polarity determines the
 * active state of the PWM signal (high or low). If the polarity is set to 1, the PWM signal
 * is active high; if set to 0, the PWM signal is active low.
 *
 * @param pwm Pointer to the PWM controller structure.
 * @param channel The PWM channel number (0-15) to configure.
 * @param polarity The desired polarity: 1 for active high, 0 for active low.
 */
static inline void sunxi_pwm_set_porality(sunxi_pwm_t *pwm, int channel, sunxi_pwm_polarity_t polarity)
{
	if (sunxi_pwm_validate_channel(pwm, channel))
		return;
	if ((int)polarity < PWM_POLARITY_INVERSED || (int)polarity > PWM_POLARITY_NORMAL)
		return;

	uint32_t reg_addr = pwm->base + PWM_PCR;
	reg_addr += PWM_REG_CHN_OFFSET * channel;
	if (polarity) {
		setbits_le32(reg_addr, BIT(PWM_ACT_STA_SHIFT));
	} else {
		clrbits_le32(reg_addr, BIT(PWM_ACT_STA_SHIFT));
	}
}

/**
 * @brief Get the PCC register offset for a given PWM channel.
 *
 * This function returns the corresponding PWM PCCR register offset based on the input channel.
 * The channels are mapped to the register offsets in a predefined array. If the provided channel
 * is out of the valid range, the function returns the offset for channel 0.
 *
 * @param channel The PWM channel number (0-15).
 *
 * @return The corresponding PCC register offset for the given channel.
 *
 * @note This function assumes that the channel is within a valid range (0-15). If the channel is
 *       outside this range, the default value of PWM_PCCR01 is returned.
 */
static inline uint32_t sunxi_pwm_get_pccr_reg_offset(uint32_t channel)
{
	static const uint32_t pccr_regs[] = {
		PWM_PCCR01, PWM_PCCR01, // channel 0, 1
		PWM_PCCR23, PWM_PCCR23, // channel 2, 3
		PWM_PCCR45, PWM_PCCR45, // channel 4, 5
		PWM_PCCR67, PWM_PCCR67, // channel 6, 7
		PWM_PCCR89, PWM_PCCR89, // channel 8, 9
		PWM_PCCRab, PWM_PCCRab, // channel a, b
		PWM_PCCRcd, PWM_PCCRcd, // channel c, d
		PWM_PCCRef, PWM_PCCRef, // channel e, f
	};
	if (channel < sizeof(pccr_regs) / sizeof(pccr_regs[0])) {
		return pccr_regs[channel];
	}
	return PWM_PCCR01;
}

/**
 * @brief Get the register offset for the PWM dead zone control register (PDZCR) for a specific channel.
 *
 * This function maps each PWM channel to its corresponding PDZCR register offset. The function returns
 * the register offset based on the given channel number. The mapping for channels is as follows:
 * - Channel 0 and 1 -> PWM_PDZCR01
 * - Channel 2 and 3 -> PWM_PDZCR23
 * - Channel 4 and 5 -> PWM_PDZCR45
 * - Channel 6 and 7 -> PWM_PDZCR67
 * - Channel 8 and 9 -> PWM_PDZCR89
 * - Channel a and b -> PWM_PDZCRab
 * - Channel c and d -> PWM_PDZCRcd
 * - Channel e and f -> PWM_PDZCRef
 *
 * @param channel The PWM channel number (0-15).
 *
 * @return The register offset corresponding to the given PWM channel.
 *         Returns PWM_PDZCR01 for invalid channel numbers.
 */
static inline uint32_t sunxi_pwm_get_pdzcr_reg_offset(uint32_t channel)
{
	static const uint32_t pdzcr_regs[] = {
		PWM_PDZCR01, PWM_PDZCR01, // channel 0, 1
		PWM_PDZCR23, PWM_PDZCR23, // channel 2, 3
		PWM_PDZCR45, PWM_PDZCR45, // channel 4, 5
		PWM_PDZCR67, PWM_PDZCR67, // channel 6, 7
		PWM_PDZCR89, PWM_PDZCR89, // channel 8, 9
		PWM_PDZCRab, PWM_PDZCRab, // channel a, b
		PWM_PDZCRcd, PWM_PDZCRcd, // channel c, d
		PWM_PDZCRef, PWM_PDZCRef, // channel e, f
	};
	if (channel < sizeof(pdzcr_regs) / sizeof(pdzcr_regs[0])) {
		return pdzcr_regs[channel];
	}
	return PWM_PDZCR01;
}

/**
 * @brief Configures a single PWM channel with the given configuration parameters.
 *
 * This function sets up the PWM configuration for a specific channel. It configures the clock source,
 * the period, duty cycle, polarity, and other related settings based on the provided configuration.
 * It also adjusts the frequency and duty cycle to ensure they are within valid ranges and applies the
 * necessary register settings.
 *
 * @param pwm Pointer to the `sunxi_pwm_t` structure representing the PWM controller.
 * @param channel The PWM channel number to configure.
 * @param config Pointer to a `sunxi_pwm_config_t` structure containing the PWM configuration parameters.
 *
 * @return 0 on success, -1 if there is an error in the configuration.
 *
 * @note The function configures the PWM clock source based on the period (frequency) provided in
 *       the configuration. It can select between APB or OSC clock sources, and sets the prescaler
 *       and divider values to achieve the desired frequency and duty cycle.
 *
 * @warning The `period_ns` in the configuration should not be zero and must be greater than `duty_ns`.
 */
static int sunxi_pwm_set_config_single(sunxi_pwm_t *pwm, int channel, sunxi_pwm_config_t *config)
{
	uint64_t clock_source_clk = 0;
	uint64_t entire_cycles = 256, active_cycles = 192;
	uint32_t pre_scal_id = 0, div_m = 0, prescale = 0;
	bool found = false;

	if (sunxi_pwm_validate_channel(pwm, channel) || sunxi_pwm_validate_config(config))
		return -1;

	pr_debug("PWM: period_ns = %ld\n", config->period_ns);
	pr_debug("PWM: duty_ns = %ld\n", config->duty_ns);
	pr_debug("PWM: polarity = %d\n", config->polarity);
	pr_debug("PWM: channel = %d\n", channel);

	sunxi_pwm_set_porality(pwm, channel, config->polarity);

	if (config->period_ns > 0 && config->period_ns <= 10) {
		/* if freq more then 100M, then direct output APB clock, set by pass. */
		/* clk_gating set */
		setbits_le32(pwm->base + PWM_PCGR, BIT(channel));
		/* clk_bypass set */
		setbits_le32(pwm->base + PWM_PCGR, BIT(PWM_CLK_BYPASS_SHIFT + channel));
		/* select clk source to APB */
		setbits_le32(pwm->base + sunxi_pwm_get_pccr_reg_offset(channel), BIT(PWM_CLK_SRC_SHIFT));
		sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channel), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_APB);
		goto set_done;
	}

	if (config->period_ns > 10 && config->period_ns <= 334) {
		/* if freq between 3M~100M, then select APB as clock */
		clock_source_clk = pwm->clk_src.clk_src_apb;
		/* select clk source to APB */
		sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channel), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_APB);
	} else if (config->period_ns > 334) {
		/* if freq < 3M, then select OSC clock */
		clock_source_clk = pwm->clk_src.clk_src_hosc;
		/* select clk source to OSC */
		sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channel), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_OSC);
	}
	if (clock_source_clk == 0U) {
		return -1;
	}

	clock_source_clk *= config->period_ns;
	sunxi_pwm_do_div(clock_source_clk, TIME_1_SECOND);

	entire_cycles = clock_source_clk;
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		if (entire_cycles <= (1U << PWM_PERIOD_CYCLES_WIDTH)) {
			found = true;
			break;
		}
		for (prescale = 0; prescale < PRESCALE_MAX + 1; prescale++) {
			entire_cycles = (clock_source_clk / pre_scal[pre_scal_id][1]) / (prescale + 1);
			if (entire_cycles <= (1U << PWM_PERIOD_CYCLES_WIDTH)) {
				div_m = pre_scal[pre_scal_id][0];
				found = true;
				break;
			}
		}
	}
	if (!found) {
		return -1;
	}

	clock_source_clk = entire_cycles * config->duty_ns;
	sunxi_pwm_do_div(clock_source_clk, config->period_ns);
	active_cycles = clock_source_clk;
	if (entire_cycles == 0)
		entire_cycles++;
	if (entire_cycles > (1U << PWM_PERIOD_CYCLES_WIDTH) || active_cycles > ((1U << PWM_ACT_CYCLES_WIDTH) - 1U)) {
		return -1;
	}

	/* config clk div_m */
	sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channel), PWM_DIV_M_SHIFT, PWM_DIV_M_WIDTH, div_m);

	/* config gating */
	setbits_le32(pwm->base + PWM_PCGR, BIT(channel));

	/* config prescal */
	sunxi_pwm_reg_set(pwm->base + PWM_PCR + PWM_REG_CHN_OFFSET * channel, PWM_PRESCAL_SHIFT, PWM_PRESCAL_WIDTH, prescale);

	/* config active cycles */
	sunxi_pwm_reg_set(pwm->base + PWM_PPR + PWM_REG_CHN_OFFSET * channel, PWM_ACT_CYCLES_SHIFT, PWM_ACT_CYCLES_WIDTH, active_cycles);

	/* config period cycles */
	sunxi_pwm_reg_set(pwm->base + PWM_PPR + PWM_REG_CHN_OFFSET * channel, PWM_PERIOD_CYCLES_SHIFT, PWM_PERIOD_CYCLES_WIDTH, (entire_cycles - 1));

set_done:
	sunxi_pwm_enable_controller(pwm, channel);
	sunxi_pwm_gpio_init(pwm, channel);
	return 0;
}

/**
 * @brief Configure and bind a PWM channel with specific settings.
 *
 * This function configures the PWM settings for the specified channel, binds it with
 * another channel, and sets up various parameters such as clock source, duty cycle,
 * dead time, active cycles, and period cycles. It also handles the configuration
 * for pulse mode and GPIO initialization for the PWM output.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The PWM channel to configure (0-based index).
 * @param config Pointer to the PWM configuration structure.
 *
 * @return 0 on success, -1 on error (e.g., invalid duty time or dead zone).
 */
static int sunxi_pwm_set_config_bind(sunxi_pwm_t *pwm, int channel, sunxi_pwm_config_t *config)
{
	uint64_t clock_source_clk = 0, pwm_clk_freq = 0, clk = 0;
	uint64_t entire_cycles = 256, active_cycles = 192;
	uint32_t pre_scal_id = 0, div_m = 0, prescale = 0, dead_time = 0;
	uint32_t reg_val = 0x0, reg_dead_time = 0x0;
	int channels[PWM_BIND_NUM] = { 0 };
	bool found = false;

	if (sunxi_pwm_validate_channel(pwm, channel) || sunxi_pwm_validate_config(config))
		return -1;

	channels[0] = channel;
	if (pwm->channel[channel].bind_channel >= SUNXI_PWM_CHANNEL_MAX || pwm->channel[channel].bind_channel == (uint32_t)channel ||
	    !(pwm->channel_mask & BIT(pwm->channel[channel].bind_channel))) {
		return -1;
	}
	channels[1] = (int)pwm->channel[channel].bind_channel;
	dead_time = pwm->channel[channel].dead_time;

	sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pdzcr_reg_offset(channels[0]), PWM_DZ_EN_SHIFT, PWM_DZ_EN_WIDTH, 0x1);

	reg_val = readl(pwm->base + sunxi_pwm_get_pdzcr_reg_offset(channels[0]));
	if (config->duty_ns < dead_time)
		return -1;
	if (reg_val == 0U)
		return -1;

	for (int i = 0; i < PWM_BIND_NUM; i++) {
		if (config->period_ns > 0 && config->period_ns <= 10) {
			/* if freq more then 100M, then direct output APB clock, set by pass. */
			clock_source_clk = pwm->clk_src.clk_src_apb;
			/* clk_gating set */
			setbits_le32(pwm->base + PWM_PCGR, BIT(channels[i]));
			/* clk_bypass set */
			setbits_le32(pwm->base + PWM_PCGR, BIT(PWM_CLK_BYPASS_SHIFT + channels[i]));
			/* select clk source to APB */
			setbits_le32(pwm->base + sunxi_pwm_get_pccr_reg_offset(channels[i]), BIT(PWM_CLK_SRC_SHIFT));
			sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channels[i]), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_APB);
		} else if (config->period_ns > 10 && config->period_ns <= 334) {
			/* if freq between 3M~100M, then select APB as clock */
			clock_source_clk = pwm->clk_src.clk_src_apb;
			/* select clk source to APB */
			sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channels[i]), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_APB);
		} else if (config->period_ns > 334) {
			/* if freq < 3M, then select OSC clock */
			clock_source_clk = pwm->clk_src.clk_src_hosc;
			/* select clk source to OSC */
			sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channels[i]), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_OSC);
		}
	}
	if (clock_source_clk == 0U) {
		return -1;
	}

	clk = clock_source_clk * config->period_ns;
	sunxi_pwm_do_div(clk, TIME_1_SECOND);
	entire_cycles = clk;

	pwm_clk_freq = clock_source_clk * dead_time;
	sunxi_pwm_do_div(pwm_clk_freq, TIME_1_SECOND);
	reg_dead_time = pwm_clk_freq;

	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		if (entire_cycles <= (1U << PWM_PERIOD_CYCLES_WIDTH) && reg_dead_time <= ((1U << PWM_PDZINTV_WIDTH) - 1U)) {
			found = true;
			break;
		}
		for (prescale = 0; prescale < PRESCALE_MAX + 1; prescale++) {
			entire_cycles = (clk / pre_scal[pre_scal_id][1]) / (prescale + 1);
			reg_dead_time = pwm_clk_freq;
			sunxi_pwm_do_div(reg_dead_time, pre_scal[pre_scal_id][1] * (prescale + 1));
			if (entire_cycles <= (1U << PWM_PERIOD_CYCLES_WIDTH) && reg_dead_time <= ((1U << PWM_PDZINTV_WIDTH) - 1U)) {
				div_m = pre_scal[pre_scal_id][0];
				found = true;
				break;
			}
		}
	}
	if (!found) {
		return -1;
	}

	clk = entire_cycles * config->duty_ns;
	sunxi_pwm_do_div(clk, config->period_ns);
	active_cycles = clk;

	if (entire_cycles == 0)
		entire_cycles++;
	if (entire_cycles > (1U << PWM_PERIOD_CYCLES_WIDTH) || active_cycles > ((1U << PWM_ACT_CYCLES_WIDTH) - 1U)) {
		return -1;
	}

	for (int i = 0; i < PWM_BIND_NUM; i++) {
		/* config clk div_m */
		sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channels[i]), PWM_DIV_M_SHIFT, PWM_DIV_M_WIDTH, div_m);

		/* config gating */
		setbits_le32(pwm->base + PWM_PCGR, BIT(channels[i]));

		/* config prescal */
		sunxi_pwm_reg_set(pwm->base + PWM_PCR + PWM_REG_CHN_OFFSET * channels[i], PWM_PRESCAL_SHIFT, PWM_PRESCAL_WIDTH, prescale);

		/* config active cycles */
		sunxi_pwm_reg_set(pwm->base + PWM_PPR + PWM_REG_CHN_OFFSET * channels[i], PWM_ACT_CYCLES_SHIFT, PWM_ACT_CYCLES_WIDTH, active_cycles);

		/* config period cycles */
		sunxi_pwm_reg_set(pwm->base + PWM_PPR + PWM_REG_CHN_OFFSET * channels[i], PWM_PERIOD_CYCLES_SHIFT, PWM_PERIOD_CYCLES_WIDTH, (entire_cycles - 1));

		/* init gpio */
		sunxi_pwm_gpio_init(pwm, channels[i]);

		if (config->pwm_mode == PWM_MODE_PLUSE) {
			/* config pluse mode */
			sunxi_pwm_reg_set(pwm->base + PWM_PCR + PWM_REG_CHN_OFFSET * channels[i], PWM_PULSE_SHIFT, PWM_PULSE_WIDTH, 0x1);
			/* config pulse num */
			sunxi_pwm_reg_set(pwm->base + PWM_PCR + PWM_REG_CHN_OFFSET * channels[i], PWM_PULSE_NUM_SHIFT, PWM_PULSE_NUM_WIDTH, (config->pluse_count - 1));
			/* enable pulse start */
			sunxi_pwm_reg_set(pwm->base + PWM_PCR + PWM_REG_CHN_OFFSET * channels[i], PWM_PULSE_START_SHIFT, PWM_PULSE_START_WIDTH, 0x1);
		}
	}

	/* config dead zone, one config for two pwm */
	sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pdzcr_reg_offset(channels[0]), PWM_PDZINTV_SHIFT, PWM_PDZINTV_WIDTH, reg_dead_time);

	/* pwm set channels[0] polarity */
	sunxi_pwm_set_porality(pwm, channels[0], config->polarity);
	/* channels[1]'s polarity opposite to channels[0]'s polarity */
	sunxi_pwm_set_porality(pwm, channels[1], !config->polarity);

	for (int i = 0; i < PWM_BIND_NUM; i++)
		sunxi_pwm_enable_controller(pwm, channels[i]);

	return 0;
}

/**
 * @brief Release a single PWM channel.
 *
 * This function handles the release of a single PWM channel. It disables the PWM controller for the
 * specified channel, closes the clock gate, and resets the clock source to the oscillator.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The PWM channel to release (0-based index).
 *
 * @return 0 on success.
 */
static int sunxi_pwm_release_single(sunxi_pwm_t *pwm, int channel)
{
	if (sunxi_pwm_validate_channel(pwm, channel))
		return -1;

	/* disable controller */
	sunxi_pwm_disable_controller(pwm, channel);

	/* Close gate */
	clrbits_le32(pwm->base + PWM_PCGR, BIT(channel));

	/* clear clk src to osc */
	sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channel), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_OSC);

	return 0;
}

/**
 * @brief Release a bound PWM channel.
 *
 * This function handles the release of a bound PWM channel. It disables the clock for both the
 * primary and bound channels, resets the clock source to the oscillator, and clears the
 * corresponding dead zone control register for the first channel in the binding.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The primary PWM channel to release (0-based index).
 *
 * @return 0 on success.
 */
static int sunxi_pwm_release_bind(sunxi_pwm_t *pwm, int channel)
{
	int channels[PWM_BIND_NUM] = { 0 };

	if (sunxi_pwm_validate_channel(pwm, channel))
		return -1;

	channels[0] = channel;
	if (pwm->channel[channel].bind_channel >= SUNXI_PWM_CHANNEL_MAX || pwm->channel[channel].bind_channel == (uint32_t)channel ||
	    !(pwm->channel_mask & BIT(pwm->channel[channel].bind_channel))) {
		return -1;
	}
	channels[1] = (int)pwm->channel[channel].bind_channel;

	for (int i = 0; i < PWM_BIND_NUM; i++) {
		/* Close gate */
		clrbits_le32(pwm->base + PWM_PCGR, BIT(channels[i]));
		/* clear clk src to osc */
		sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pccr_reg_offset(channels[i]), PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, PWM_CLK_SRC_OSC);
	}

	/* clear pdzcr select */
	sunxi_pwm_reg_set(pwm->base + sunxi_pwm_get_pdzcr_reg_offset(channels[0]), PWM_DZ_EN_SHIFT, PWM_DZ_EN_WIDTH, 0x0);

	return 0;
}

/**
 * @brief Initialize the PWM instance.
 *
 * This function initializes the PWM instance by setting up the necessary clocks and
 * marking the PWM as initialized (status set to true).
 *
 * @param pwm Pointer to the PWM instance structure.
 */
void sunxi_pwm_init(sunxi_pwm_t *pwm)
{
	if (pwm == NULL) {
		pr_err("PWM: cannot initialize NULL controller\n");
		return;
	}
	sunxi_pwm_clk_init(pwm);
	pwm->status = true;
}

/**
 * @brief Deinitialize the PWM instance.
 *
 * This function deinitializes the PWM instance by deactivating the clocks and
 * re-initializing the GPIOs for each channel. It also marks the PWM as uninitialized
 * (status set to false).
 *
 * @param pwm Pointer to the PWM instance structure.
 */
void sunxi_pwm_deinit(sunxi_pwm_t *pwm)
{
	if (pwm == NULL) {
		pr_err("PWM: cannot deinitialize NULL controller\n");
		return;
	}
	sunxi_pwm_clk_deinit(pwm);
	pwm->status = false;
}

/**
 * @brief Set configuration for a PWM channel.
 *
 * This function sets the configuration for the specified PWM channel. It checks if the PWM is initialized,
 * validates the channel index, and then applies the configuration either for a bound channel or a single channel
 * based on the current channel mode.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The PWM channel to configure (0-based index).
 * @param config Pointer to the PWM configuration structure containing the settings.
 *
 * @return 0 on success, -1 if an error occurs (e.g., PWM not initialized, invalid channel index).
 */
int sunxi_pwm_set_config(sunxi_pwm_t *pwm, int channel, sunxi_pwm_config_t *config)
{
	int ret;

	if (pwm == NULL) {
		pr_err("PWM: controller is NULL\n");
		return -1;
	}
	if (!pwm->status) {
		pr_err("PWM: controller is not initialized\n");
		return -1;
	}
	if (sunxi_pwm_validate_config(config)) {
		pr_err("PWM: invalid configuration\n");
		return -1;
	}
	if (sunxi_pwm_validate_channel(pwm, channel)) {
		pr_err("PWM: invalid channel %d\n", channel);
		return -1;
	}

	if (pwm->channel[channel].channel_mode == PWM_CHANNEL_BIND) {
		ret = sunxi_pwm_set_config_bind(pwm, channel, config);
	} else {
		ret = sunxi_pwm_set_config_single(pwm, channel, config);
	}
	if (ret)
		pr_err("PWM: failed to configure channel %d\n", channel);
	return ret;
}

/**
 * @brief Release the PWM channel.
 *
 * This function releases the specified PWM channel. It checks if the PWM is initialized,
 * validates the channel index, and then either releases a bound channel or a single channel
 * based on the current channel mode.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The PWM channel to release (0-based index).
 *
 * @return 0 on success, -1 if an error occurs (e.g., PWM not initialized, invalid channel index).
 */
int sunxi_pwm_release(sunxi_pwm_t *pwm, int channel)
{
	int ret;

	if (pwm == NULL) {
		pr_err("PWM: controller is NULL\n");
		return -1;
	}
	if (!pwm->status) {
		pr_err("PWM: controller is not initialized\n");
		return -1;
	}
	if (sunxi_pwm_validate_channel(pwm, channel)) {
		pr_err("PWM: invalid channel %d\n", channel);
		return -1;
	}

	if (pwm->channel[channel].channel_mode == PWM_CHANNEL_BIND) {
		ret = sunxi_pwm_release_bind(pwm, channel);
	} else {
		ret = sunxi_pwm_release_single(pwm, channel);
	}
	if (ret)
		pr_err("PWM: failed to release channel %d\n", channel);
	return ret;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-pwm");
