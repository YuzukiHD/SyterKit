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

	{ "bldo2", 500, 3500, AXP2101_BLDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 5, 0,
	{ {500, 3500, 100}, } },

	{ "cpusldo", 500, 1400, AXP2101_CPUSLDO_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 6, 0,
	{ {500, 1400, 50}, } },

	{ "dldo1", 500, 3300, AXP2101_DLDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 7, 0,
	{ {500, 3300, 100}, } },

	{ "dldo2", 500, 1400, AXP2101_DLDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL3, 0, 0,
	{ {500, 1400, 50}, } },
};
/* clang-format on */

int pmu_axp2101_init(sunxi_i2c_t *i2c_dev) {
	uint8_t axp_val;
	uint8_t reg_value;
	int ret;

	if (!i2c_dev->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if (ret = sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_VERSION, &axp_val)) {
		printk_warning("PMU: Probe target device AXP2101 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	if (axp_val == AXP2101_CHIP_ID || axp_val == AXP2101_CHIP_ID_B) {
		printk_info("PMU: Found AXP2101 PMU\n");

		/* limit charge current to 300mA */
		reg_value = 0x9;
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_CHARGE1, reg_value);

		/* limit run current to 2A */
		reg_value = 0x5;
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_VBUS_CUR_SET, reg_value);

		/* enable vbus adc channel */
		if (axp_val != AXP2101_CHIP_ID_B) {
			reg_value = 0x40;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6, reg_value);
		}

		/* set dcdc1 & dcdc3 & dcdc2  & dcdc4 pwm mode */
		sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_OUTPUT_CTL1, &reg_value);
		reg_value |= ((1 << 2) | (1 << 4) | (1 << 3) | (1 << 5));
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_OUTPUT_CTL1, reg_value);

		/* pmu disable soften3 signal */
		if (axp_val != AXP2101_CHIP_ID_B) {
			reg_value = 0x00;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_TWI_ADDR_EXT, reg_value);
			reg_value = 0x06;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_EFUS_OP_CFG, reg_value);
			reg_value = 0x04;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_EFREQ_CTRL, reg_value);
			reg_value = 0x01;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_TWI_ADDR_EXT, reg_value);
			reg_value = 0x30;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_SELLP_CFG, reg_value);
			reg_value = 0x00;
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_TWI_ADDR_EXT, reg_value);
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_EFREQ_CTRL, reg_value);
			sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_EFUS_OP_CFG, reg_value);
		}

		/* pmu set vsys min */
		sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_VSYS_MIN, &reg_value);
		reg_value &= ~(0x7 << 4);
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_VSYS_MIN, reg_value);

		/* pmu set vimdpm cfg */
		sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_VBUS_VOL_SET, &reg_value);
		reg_value &= ~(0xf << 0);
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_VBUS_VOL_SET, reg_value);

		/* pmu reset enable */
		sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_OFF_CTL, &reg_value);
		reg_value |= (3 << 2);
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_OFF_CTL, reg_value);

		/* pmu pwroff enable */
		sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_PWEON_PWEOFF_EN, &reg_value);
		reg_value |= (1 << 1);
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_PWEON_PWEOFF_EN, reg_value);

		/* pmu dcdc1 pwroff enable */
		sunxi_i2c_read(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_DCDC_PWEOFF_EN, &reg_value);
		reg_value &= ~(1 << 0);
		sunxi_i2c_write(i2c_dev, AXP2101_RUNTIME_ADDR, AXP2101_DCDC_PWEOFF_EN, reg_value);

		return AXP2101_CHIP_ID;
	}
	return -1;
}

int pmu_axp2101_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff) {
	return axp_set_vol(i2c_dev, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP2101_RUNTIME_ADDR);
}

int pmu_axp2101_get_vol(sunxi_i2c_t *i2c_dev, char *name) {
	return axp_get_vol(i2c_dev, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP2101_RUNTIME_ADDR);
}

void pmu_axp2101_dump(sunxi_i2c_t *i2c_dev) {
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) { printk_debug("PMU: AXP2101 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp2101_get_vol(i2c_dev, axp_ctrl_tbl[i].name)); }
}
