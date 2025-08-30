/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <pmu/axp.h>

/* clang-format off */
static axp_contrl_info axp_ctrl_tbl[] = {
	{ "dcdc1", 500, 3400, AXP333_DC1OUT_VOL, 0x7f, AXP333_DCDC_LDO_POWER_ON_OFF_CTL1, 0, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1600, 3400, 100} } },

	{ "dcdc2", 500, 1840, AXP333_DC2OUT_VOL, 0x7f, AXP333_DCDC_LDO_POWER_ON_OFF_CTL1, 1, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, } },

	{ "dcdc3", 500, 3400, AXP333_DC3OUT_VOL, 0x7f, AXP333_DCDC_LDO_POWER_ON_OFF_CTL1, 2, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, {3100, 3400, 100} } },

	{ "aldo1", 500, 3500, AXP333_ALDO1OUT_VOL, 0x1f, AXP333_DCDC_LDO_POWER_ON_OFF_CTL1, 3, 0,
	{ {500, 3500, 100}, } },

	{ "aldo2", 500, 3500, AXP333_ALDO2OUT_VOL, 0x1f, AXP333_DCDC_LDO_POWER_ON_OFF_CTL1, 4, 0,
	{ {500, 3500, 100}, } },
};
/* clang-format on */

int pmu_axp333_init(sunxi_i2c_t *i2c_dev) {
	uint8_t axp_val;
	uint8_t reg_value;
	int ret;

	if (!i2c_dev->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if (ret = sunxi_i2c_read(i2c_dev, AXP333_RUNTIME_ADDR, AXP333_IC_TYPE, &axp_val)) {
		printk_warning("PMU: Probe target device AXP333 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	if (axp_val == AXP333_CHIP_ID) {
		printk_info("PMU: Found AXP333 PMU\n");

		reg_value = 0;
		sunxi_i2c_read(i2c_dev, AXP333_RUNTIME_ADDR, AXP333_CHIP_ID_REG, &reg_value);
		reg_value |= 0x10;
		sunxi_i2c_write(i2c_dev, AXP333_RUNTIME_ADDR, AXP333_CHIP_ID_REG, reg_value);

		return AXP333_CHIP_ID;
	}
	return -1;
}

int pmu_axp333_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff) {
	return axp_set_vol(i2c_dev, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP333_RUNTIME_ADDR);
}

int pmu_axp333_get_vol(sunxi_i2c_t *i2c_dev, char *name) {
	return axp_get_vol(i2c_dev, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP333_RUNTIME_ADDR);
}

void pmu_axp333_dump(sunxi_i2c_t *i2c_dev) {
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) {
		printk_debug("PMU: AXP333 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp333_get_vol(i2c_dev, axp_ctrl_tbl[i].name));
	}
}
