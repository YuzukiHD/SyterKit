/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/i2c-dt.h>
#include <drivers/pmu/axp.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_i2c_t i2c0 = {0};
	sunxi_i2c_t i2c1 = {0};
	axp_pmu_t axp2202 = {0};
	axp_pmu_t axp1530 = {0};

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_i2c_dt_read_alias(&i2c0, "i2c0"));
	TEST_EQ(0x3000U, i2c0.base);
	TEST_EQ(SUNXI_R_I2C0, i2c0.id);
	TEST_EQ(400000U, i2c0.speed);
	TEST_EQ(24000000U, i2c0.i2c_clk.parent_clk);
	TEST_EQ(0x4000U, i2c0.i2c_clk.gate_reg_base);
	TEST_EQ(0U, i2c0.i2c_clk.gate_reg_offset);
	TEST_EQ(0x4000U, i2c0.i2c_clk.rst_reg_base);
	TEST_EQ(16U, i2c0.i2c_clk.rst_reg_offset);
	TEST_EQ(GPIO_PIN(GPIO_PORTL, 0), i2c0.gpio.gpio_scl.pin);
	TEST_EQ(GPIO_PERIPH_MUX2, i2c0.gpio.gpio_scl.mux);
	TEST_EQ(0x07022000U, i2c0.gpio.gpio_scl.base);
	TEST_EQ(0, i2c0.gpio.gpio_scl.bank);
	TEST_EQ(GPIO_PIN(GPIO_PORTL, 1), i2c0.gpio.gpio_sda.pin);
	TEST_EQ(GPIO_PERIPH_MUX2, i2c0.gpio.gpio_sda.mux);
	TEST_ASSERT(!i2c0.status);

	TEST_EQ(DRIVER_OK, sunxi_i2c_dt_read_alias(&i2c1, "i2c1"));
	TEST_EQ(0x5000U, i2c1.base);
	TEST_EQ(SUNXI_I2C1, i2c1.id);
	TEST_EQ(100000U, i2c1.speed);
	TEST_EQ(0x02000000U, i2c1.gpio.gpio_scl.base);
	TEST_EQ(1, i2c1.gpio.gpio_scl.bank);

	TEST_EQ(DRIVER_OK, pmu_axp2202_config(&axp2202, &i2c0));
	TEST_ASSERT(axp2202.i2c == &i2c0);
	TEST_EQ(AXP_PMU_AXP2202, axp2202.type);
	TEST_EQ(0x34U, axp2202.address);
	TEST_EQ(0x35U, axp2202.fallback_address);
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_i2c_dt_read_alias(&i2c0, NULL));

	TEST_EQ(DRIVER_OK, pmu_axp1530_config(&axp1530, &i2c1));
	TEST_ASSERT(axp1530.i2c == &i2c1);
	TEST_EQ(AXP_PMU_AXP1530, axp1530.type);
	TEST_EQ(0x36U, axp1530.address);
	TEST_EQ(0U, axp1530.fallback_address);
}
