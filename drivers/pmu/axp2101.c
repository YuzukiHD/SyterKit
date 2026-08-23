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
    { "dcdc1", 500, 3400, AXP2101_DC1OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 0, 0,
	{ {500, 3400, 100}, } },

	{ "dcdc2", 500, 1540, AXP2101_DC2OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 1, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc3", 500, 3400, AXP2101_DC3OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 2, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1600, 3400, 100}, } },

	{ "dcdc4", 500, 1840, AXP2101_DC4OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 3, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, } },

	{ "dcdc5", 500, 3700, AXP2101_DC5OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 4, 0,
	{ {500, 3700, 100}, } },

	{ "aldo1", 500, 3500, AXP2101_ALDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 0, 0,
	{ {500, 3500, 100}, } },

	{ "aldo2", 500, 3500, AXP2101_ALDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 1, 0,
	{ {500, 3500, 100}, } },

	{ "aldo3", 500, 3500, AXP2101_ALDO3OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 2, 0,
	{ {500, 3500, 100}, } },

	{ "aldo4", 500, 3500, AXP2101_ALDO4OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 3, 0,
	{ {500, 3500, 100}, } },

	{ "bldo1", 500, 3500, AXP2101_BLDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 4, 0,
	{ {500, 3500, 100}, } },

	{ "bldo2", 500, 3500, AXP2101_BLDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 5, 0,
	{ {500, 3500, 100}, } },

	{ "cpusldo", 500, 1400, AXP2101_CPUSLDO_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 6, 0,
	{ {500, 1400, 50}, } },

	{ "dldo1", 500, 3300, AXP2101_DLDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 7, 0,
	{ {500, 3300, 100}, } },

	{ "dldo2", 500, 1400, AXP2101_DLDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL3, 0, 0,
	{ {500, 1400, 50}, } },
};
/* clang-format on */

int pmu_axp2101_config(axp_pmu_t *pmu, sunxi_i2c_t *i2c) {
	return sunxi_pmu_config(pmu, i2c, AXP_PMU_AXP2101,
				AXP2101_RUNTIME_ADDR, 0U);
}

int pmu_axp2101_init(axp_pmu_t *pmu) {
	uint8_t axp_val;
	uint8_t reg_value;
	int ret;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2101) ||
	    !pmu->i2c->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if ((ret = sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_VERSION, &axp_val))) {
		printk_warning("PMU: Probe target device AXP2101 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	if (axp_val == AXP2101_CHIP_ID || axp_val == AXP2101_CHIP_ID_B) {
		printk_info("PMU: Found AXP2101 PMU\n");

		/* limit charge current to 300mA */
		reg_value = 0x9;
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_CHARGE1, reg_value))
			return -1;

		/* limit run current to 2A */
		reg_value = 0x5;
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_VBUS_CUR_SET, reg_value))
			return -1;

		/* enable vbus adc channel */
		if (axp_val != AXP2101_CHIP_ID_B) {
			reg_value = 0x40;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_BAT_AVERVOL_H6, reg_value))
				return -1;
		}

		/* set dcdc1 & dcdc3 & dcdc2  & dcdc4 pwm mode */
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP2101_OUTPUT_CTL1, &reg_value))
			return -1;
		reg_value |= ((1 << 2) | (1 << 4) | (1 << 3) | (1 << 5));
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_OUTPUT_CTL1, reg_value))
			return -1;

		/* pmu disable soften3 signal */
		if (axp_val != AXP2101_CHIP_ID_B) {
			reg_value = 0x00;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_TWI_ADDR_EXT, reg_value))
				return -1;
			reg_value = 0x06;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_EFUS_OP_CFG, reg_value))
				return -1;
			reg_value = 0x04;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_EFREQ_CTRL, reg_value))
				return -1;
			reg_value = 0x01;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_TWI_ADDR_EXT, reg_value))
				return -1;
			reg_value = 0x30;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_SELLP_CFG, reg_value))
				return -1;
			reg_value = 0x00;
			if (sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_TWI_ADDR_EXT, reg_value) ||
			    sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_EFREQ_CTRL, reg_value) ||
			    sunxi_i2c_write(pmu->i2c, pmu->address,
					    AXP2101_EFUS_OP_CFG, reg_value))
				return -1;
		}

		/* pmu set vsys min */
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP2101_VSYS_MIN, &reg_value))
			return -1;
		reg_value &= ~(0x7 << 4);
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_VSYS_MIN, reg_value))
			return -1;

		/* pmu set vimdpm cfg */
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP2101_VBUS_VOL_SET, &reg_value))
			return -1;
		reg_value &= ~(0xf << 0);
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_VBUS_VOL_SET, reg_value))
			return -1;

		/* pmu reset enable */
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP2101_OFF_CTL, &reg_value))
			return -1;
		reg_value |= (3 << 2);
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_OFF_CTL, reg_value))
			return -1;

		/* pmu pwroff enable */
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP2101_PWEON_PWEOFF_EN, &reg_value))
			return -1;
		reg_value |= (1 << 1);
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_PWEON_PWEOFF_EN, reg_value))
			return -1;

		/* pmu dcdc1 pwroff enable */
		if (sunxi_i2c_read(pmu->i2c, pmu->address,
				   AXP2101_DCDC_PWEOFF_EN, &reg_value))
			return -1;
		reg_value &= ~(1 << 0);
		if (sunxi_i2c_write(pmu->i2c, pmu->address,
				    AXP2101_DCDC_PWEOFF_EN, reg_value))
			return -1;

		return AXP2101_CHIP_ID;
	}
	return -1;
}

int pmu_axp2101_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff) {
	return axp_set_vol(pmu, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

int pmu_axp2101_get_vol(axp_pmu_t *pmu, char *name) {
	return axp_get_vol(pmu, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

void pmu_axp2101_dump(axp_pmu_t *pmu) {
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) { printk_debug("PMU: AXP2101 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp2101_get_vol(pmu, axp_ctrl_tbl[i].name)); }
}
