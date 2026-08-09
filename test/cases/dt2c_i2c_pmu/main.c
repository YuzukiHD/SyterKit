/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/pmu-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_i2c_t i2c0 = {0};
	sunxi_i2c_t i2c1 = {0};
	axp_pmu_t pmu0 = {0};
	axp_pmu_t pmu1 = {0};

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

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_pmu_dt_read_alias(&pmu0, "pmu0", &i2c1));
	TEST_EQ(DRIVER_OK,
		 sunxi_pmu_dt_read_alias(&pmu0, "pmu0", &i2c0));
	TEST_ASSERT(pmu0.i2c == &i2c0);
	TEST_EQ(AXP_PMU_AXP2202, pmu0.type);
	TEST_EQ(0x34U, pmu0.address);
	TEST_EQ(0x35U, pmu0.fallback_address);
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_pmu_dt_read_alias(&pmu0, "pmu-invalid-address", &i2c0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_pmu_dt_read_alias(&pmu0, "pmu-invalid-fallback", &i2c0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_pmu_dt_read_alias(&pmu0, NULL, &i2c0));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_i2c_dt_read_alias(&i2c0, NULL));

	TEST_EQ(DRIVER_OK,
		 sunxi_pmu_dt_read_alias(&pmu1, "pmu1", &i2c1));
	TEST_ASSERT(pmu1.i2c == &i2c1);
	TEST_EQ(AXP_PMU_AXP1530, pmu1.type);
	TEST_EQ(0x36U, pmu1.address);
	TEST_EQ(0U, pmu1.fallback_address);
}
