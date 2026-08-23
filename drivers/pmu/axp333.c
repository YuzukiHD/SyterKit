/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <drivers/pmu/axp.h>
#include "axp-config.h"

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

int pmu_axp333_config(axp_pmu_t *pmu, sunxi_i2c_t *i2c) {
	return sunxi_pmu_config(pmu, i2c, AXP_PMU_AXP333,
				AXP333_RUNTIME_ADDR, 0U);
}

int pmu_axp333_init(axp_pmu_t *pmu) {
	uint8_t axp_val;
	uint8_t reg_value;
	int ret;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP333) ||
	    !pmu->i2c->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if ((ret = sunxi_i2c_read(pmu->i2c, pmu->address, AXP333_IC_TYPE, &axp_val))) {
		printk_warning("PMU: Probe target device AXP333 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	if (axp_val == AXP333_CHIP_ID) {
		printk_info("PMU: Found AXP333 PMU\n");

		reg_value = 0;
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP333_CHIP_ID_REG, &reg_value))
			return -1;
		reg_value |= 0x10;
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP333_CHIP_ID_REG, reg_value))
			return -1;

		return AXP333_CHIP_ID;
	}
	return -1;
}

int pmu_axp333_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff) {
	return axp_set_vol(pmu, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

int pmu_axp333_get_vol(axp_pmu_t *pmu, char *name) {
	return axp_get_vol(pmu, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

void pmu_axp333_dump(axp_pmu_t *pmu) {
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) {
		printk_debug("PMU: AXP333 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp333_get_vol(pmu, axp_ctrl_tbl[i].name));
	}
}
