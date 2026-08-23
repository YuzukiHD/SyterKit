/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PWM_DT_H__
#define __DT_COMPATIBLE_PWM_DT_H__

#include <driver.h>
#include <drivers/pwm/pwm.h>
#include <dt-compatible/pinctrl-dt.h>

#define SUNXI_PWM_COMPATIBLE "allwinner,sunxi-pwm"
#define SUNXI_PWM_CHANNEL_CONFIG_CELLS 4U

static inline __attribute__((always_inline)) int
sunxi_pwm_dt_read_config(sunxi_pwm_t *pwm, int node) {
	const dt2c_fdt32_t *apb_clock;
	const dt2c_fdt32_t *channel_cells;
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *hosc_clock;
	const dt2c_fdt32_t *id_cells;
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *pwm_cells;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	sunxi_gpio_t gpio_controller;
	sunxi_pwm_t config = {0};
	uint32_t channel_count;
	uint32_t channel_mask = 0U;
	int channel_length;

	if (pwm == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_PWM_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	id_cells = syterkit_dt_cells(node, "allwinner,pwm-id", 1);
	pwm_cells = syterkit_dt_cells(node, "#pwm-cells", 1);
	hosc_clock = syterkit_dt_cells(node, "clock-frequency", 1);
	apb_clock = syterkit_dt_cells(node, "allwinner,apb-clock-frequency", 1);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	channel_cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,channel-config", &channel_length);
	if (reg == NULL || id_cells == NULL || pwm_cells == NULL ||
	    hosc_clock == NULL || apb_clock == NULL || clock_gate == NULL ||
	    reset == NULL || channel_cells == NULL || channel_length <= 0 ||
	    channel_length %
		    (int) (SUNXI_PWM_CHANNEL_CONFIG_CELLS *
			   sizeof(*channel_cells)) != 0)
		return DRIVER_ERROR_INVALID;

	channel_count = (uint32_t) channel_length /
			(SUNXI_PWM_CHANNEL_CONFIG_CELLS * sizeof(*channel_cells));
	pins = syterkit_dt_pinctrl_cells(node, (size_t) channel_count * 3U,
					&gpio_controller);
	if (channel_count == 0U || channel_count > SUNXI_PWM_CHANNEL_MAX ||
	    pins == NULL || dt2c_fdt32_to_cpu(pwm_cells[0]) != 3U ||
	    dt2c_fdt32_to_cpu(reg[0]) == 0U ||
	    dt2c_fdt32_to_cpu(reg[1]) == 0U ||
	    dt2c_fdt32_to_cpu(id_cells[0]) > UINT8_MAX ||
	    dt2c_fdt32_to_cpu(hosc_clock[0]) == 0U ||
	    dt2c_fdt32_to_cpu(apb_clock[0]) == 0U ||
	    dt2c_fdt32_to_cpu(clock_gate[0]) == 0U ||
	    dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U ||
	    dt2c_fdt32_to_cpu(reset[0]) == 0U ||
	    dt2c_fdt32_to_cpu(reset[1]) >= 32U)
		return DRIVER_ERROR_INVALID;

	for (uint32_t index = 0; index < channel_count; index++) {
		const dt2c_fdt32_t *entry = channel_cells +
				index * SUNXI_PWM_CHANNEL_CONFIG_CELLS;
		uint32_t channel = dt2c_fdt32_to_cpu(entry[0]);
		uint32_t mode = dt2c_fdt32_to_cpu(entry[1]);
		uint32_t bind_channel = dt2c_fdt32_to_cpu(entry[2]);
		uint32_t dead_time = dt2c_fdt32_to_cpu(entry[3]);

		if (channel >= SUNXI_PWM_CHANNEL_MAX ||
		    (channel_mask & BIT(channel)) != 0U ||
		    mode > PWM_CHANNEL_BIND ||
		    (mode == PWM_CHANNEL_SINGLE &&
		     (bind_channel != 0U || dead_time != 0U)) ||
		    (mode == PWM_CHANNEL_BIND &&
		     (bind_channel >= SUNXI_PWM_CHANNEL_MAX ||
		      bind_channel == channel || dead_time == 0U)) ||
		    !syterkit_dt_pinctrl_gpio(
				    pins, (size_t) index * 3U, &gpio_controller,
				    &config.channel[channel].pin))
			return DRIVER_ERROR_INVALID;

		config.channel[channel].channel_mode =
				(sunxi_pwm_channel_mode_t) mode;
		config.channel[channel].bind_channel = bind_channel;
		config.channel[channel].dead_time = dead_time;
		channel_mask |= BIT(channel);
	}

	for (uint32_t channel = 0; channel < SUNXI_PWM_CHANNEL_MAX; channel++) {
		uint32_t bind_channel;

		if ((channel_mask & BIT(channel)) == 0U ||
		    config.channel[channel].channel_mode != PWM_CHANNEL_BIND)
			continue;
		bind_channel = config.channel[channel].bind_channel;
		if ((channel_mask & BIT(bind_channel)) == 0U ||
		    config.channel[bind_channel].channel_mode != PWM_CHANNEL_BIND ||
		    config.channel[bind_channel].bind_channel != channel ||
		    config.channel[bind_channel].dead_time !=
			    config.channel[channel].dead_time)
			return DRIVER_ERROR_INVALID;
	}

	config.base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	config.id = (uint8_t) dt2c_fdt32_to_cpu(id_cells[0]);
	config.dt_node = node;
	config.channel_mask = channel_mask;
	config.pwm_clk.gate_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(clock_gate[0]);
	config.pwm_clk.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	config.pwm_clk.rst_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(reset[0]);
	config.pwm_clk.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	config.clk_src.clk_src_hosc = dt2c_fdt32_to_cpu(hosc_clock[0]);
	config.clk_src.clk_src_apb = dt2c_fdt32_to_cpu(apb_clock[0]);
	*pwm = config;
	SYTERKIT_DT_TRACE_NODE("pwm", node);
	SYTERKIT_DT_TRACE("pwm config base=%p id=%u channels=0x%04x hosc=%u apb=%u gate=%p:%u reset=%p:%u\n",
			 (void *) pwm->base, pwm->id, pwm->channel_mask,
			 pwm->clk_src.clk_src_hosc, pwm->clk_src.clk_src_apb,
			 (void *) pwm->pwm_clk.gate_reg_base,
			 pwm->pwm_clk.gate_reg_offset,
			 (void *) pwm->pwm_clk.rst_reg_base,
			 pwm->pwm_clk.rst_reg_offset);
	for (uint32_t channel = 0U; channel < SUNXI_PWM_CHANNEL_MAX; ++channel) {
		if ((pwm->channel_mask & BIT(channel)) == 0U)
			continue;
		SYTERKIT_DT_TRACE("pwm channel=%u mode=%u bind=%u dead_time=%u pin=%u/%u\n",
				 channel, pwm->channel[channel].channel_mode,
				 pwm->channel[channel].bind_channel,
				 pwm->channel[channel].dead_time,
				 pwm->channel[channel].pin.pin,
				 pwm->channel[channel].pin.mux);
	}
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_pwm_dt_read_alias(sunxi_pwm_t *pwm, const char *alias) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_PWM_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_pwm_dt_read_config(pwm, node);
}

#endif /* __DT_COMPATIBLE_PWM_DT_H__ */
