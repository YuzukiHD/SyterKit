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
	{ "dcdc1", 500, 3400, AXP1530_DC1OUT_VOL, 0x7f, AXP1530_OUTPUT_POWER_ON_OFF_CTL, 0, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1600, 3400, 100}, } },

	{ "dcdc2", 500, 1540, AXP1530_DC2OUT_VOL, 0x7f, AXP1530_OUTPUT_POWER_ON_OFF_CTL, 1, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc3", 500, 1840, AXP1530_DC3OUT_VOL, 0x7f, AXP1530_OUTPUT_POWER_ON_OFF_CTL, 2, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, } },

	{ "aldo1", 500, 3500, AXP1530_ALDO1OUT_VOL, 0x1f, AXP1530_OUTPUT_POWER_ON_OFF_CTL, 3, 0,
	{ {500, 3500, 100}, } },

	{ "dldo1", 500, 3500, AXP1530_DLDO1OUT_VOL, 0x1f, AXP1530_OUTPUT_POWER_ON_OFF_CTL, 4, 0,
	{ {500, 3500, 100}, } },
};
/* clang-format on */

int pmu_axp1530_init(sunxi_i2c_t *i2c_dev) {
	uint8_t axp_val;
	int ret;

	if (!i2c_dev->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if (ret = sunxi_i2c_read(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_VERSION, &axp_val)) {
		printk_warning("PMU: Probe target device AXP1530 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	switch (axp_val) {
		case AXP1530_CHIP_ID:
			printk_info("PMU: Found AXP1530 PMU\n");
			break;
		case AXP313A_CHIP_ID:
			printk_info("PMU: Found AXP313A PMU\n");
			break;
		case AXP313B_CHIP_ID:
			printk_info("PMU: Found AXP313B PMU\n");
			break;
		case AXP323_CHIP_ID:
			printk_info("PMU: Found AXP323 PMU\n");
			break;
		default:
			printk_info("PMU: Cannot found match PMU\n");
			return -1;
	}

	/* Set over temperature shutdown functtion */
	if (sunxi_i2c_read(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_POWER_DOMN_SEQUENCE, &axp_val))
		return -1;
	axp_val |= (0x1 << 1);
	if (sunxi_i2c_write(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_POWER_DOMN_SEQUENCE, axp_val))
		return -1;

	return 0;
}

int pmu_axp1530_set_dual_phase(sunxi_i2c_t *i2c_dev) {
	uint8_t axp_val;
	int ret;

	if (ret = sunxi_i2c_read(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_VERSION, &axp_val)) {
		printk_warning("PMU: Probe target device AXP1530 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	switch (axp_val) {
		case AXP323_CHIP_ID: /* Only AXP323 Support Dual phase */
			break;
		default:
			printk_info("PMU: PMU not support dual phase\n");
			return -1;
	}

	sunxi_i2c_write(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_OUTPUT_MONITOR_CONTROL, 0x1E);
	sunxi_i2c_write(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_DCDC_MODE_CTRL2, 0x02);
	sunxi_i2c_write(i2c_dev, AXP1530_RUNTIME_ADDR, AXP1530_POWER_DOMN_SEQUENCE, 0x22);

	return 0;
}

int pmu_axp1530_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff) {
	return axp_set_vol(i2c_dev, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP1530_RUNTIME_ADDR);
}

int pmu_axp1530_get_vol(sunxi_i2c_t *i2c_dev, char *name) {
	return axp_get_vol(i2c_dev, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP1530_RUNTIME_ADDR);
}

void pmu_axp1530_dump(sunxi_i2c_t *i2c_dev) {
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) { printk_debug("PMU: AXP1530 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp1530_get_vol(i2c_dev, axp_ctrl_tbl[i].name)); }
}