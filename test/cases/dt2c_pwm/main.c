/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/pwm-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_pwm_t pwm0 = { 0 };
	sunxi_pwm_t pwm1 = { 0 };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_pwm_dt_read_alias(&pwm0, "pwm0"));
	TEST_EQ(0xf000U, pwm0.base);
	TEST_EQ(0U, pwm0.id);
	TEST_EQ(24000000U, pwm0.clk_src.clk_src_hosc);
	TEST_EQ(200000000U, pwm0.clk_src.clk_src_apb);
	TEST_EQ(0x10000U, pwm0.pwm_clk.gate_reg_base);
	TEST_EQ(8U, pwm0.pwm_clk.gate_reg_offset);
	TEST_EQ(0x10100U, pwm0.pwm_clk.rst_reg_base);
	TEST_EQ(24U, pwm0.pwm_clk.rst_reg_offset);
	TEST_EQ(BIT(0) | BIT(1) | BIT(2) | BIT(3), pwm0.channel_mask);
	TEST_EQ(GPIO_PIN(GPIO_PORTE, 0), pwm0.channel[0].pin.pin);
	TEST_EQ(GPIO_PERIPH_MUX4, pwm0.channel[0].pin.mux);
	TEST_EQ(0x02000000U, pwm0.channel[0].pin.base);
	TEST_EQ(4, pwm0.channel[0].pin.bank);
	TEST_EQ(PWM_CHANNEL_BIND, pwm0.channel[2].channel_mode);
	TEST_EQ(3U, pwm0.channel[2].bind_channel);
	TEST_EQ(4000U, pwm0.channel[2].dead_time);
	TEST_EQ(PWM_CHANNEL_BIND, pwm0.channel[3].channel_mode);
	TEST_EQ(2U, pwm0.channel[3].bind_channel);
	TEST_ASSERT(!pwm0.status);

	TEST_EQ(DRIVER_OK, sunxi_pwm_dt_read_alias(&pwm1, "pwm1"));
	TEST_EQ(0x11000U, pwm1.base);
	TEST_EQ(1U, pwm1.id);
	TEST_EQ(BIT(4) | BIT(7), pwm1.channel_mask);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 4), pwm1.channel[4].pin.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, pwm1.channel[4].pin.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 7), pwm1.channel[7].pin.pin);
	TEST_ASSERT(pwm0.dt_node != pwm1.dt_node);
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_pwm_dt_read_alias(&pwm1, "missing"));
}
