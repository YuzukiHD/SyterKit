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
	{ "dcdc1", 1000, 3800, AXP8191_DC1OUT_VOL, 0x1f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 0, 0,
	{ {1000, 3800, 100}, } },

	{ "dcdc2", 500, 1540, AXP8191_DC2OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 1, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc3", 500, 1540, AXP8191_DC3OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 2, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc4", 500, 1540, AXP8191_DC4OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 3, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc5", 500, 1540, AXP8191_DC5OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 4, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1800, 2400, 20}, {2440, 2760, 100}, } },

	{ "dcdc6", 500, 2760, AXP8191_DC6OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 5, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1800, 2400, 20}, {2440, 2760, 40}, } },

	{ "dcdc7", 500, 1840, AXP8191_DC7OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 6, 0,
	{ {500, 1200, 10}, {1220, 1840, 20} } },

	{ "dcdc8", 500, 3400, AXP8191_DC8OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL1, 7, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1900, 3400, 100}, } },

	{ "dcdc9", 500, 3400, AXP8191_DC9OUT_VOL, 0x7f, AXP8191_DCDC_POWER_ON_OFF_CTL2, 0, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1900, 3400, 100}, } },

	{ "aldo1", 500, 3400, AXP8191_ALDO1OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 0, 0,
	{ {500, 3400, 100}, } },

	{ "aldo2", 500, 3400, AXP8191_ALDO2OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 1, 0,
	{ {500, 3400, 100}, } },

	{ "aldo3", 500, 3400, AXP8191_ALDO3OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 2, 0,
	{ {500, 3400, 100}, } },

	{ "aldo4", 500, 3400, AXP8191_ALDO4OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 3, 0,
	{ {500, 3400, 100}, } },

	{ "aldo5", 500, 3400, AXP8191_ALDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 4, 0,
	{ {500, 3400, 100}, } },

	{ "aldo6", 500, 3400, AXP8191_ALDO6OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 5, 0,
	{ {500, 3400, 100}, } },

	{ "bldo1", 500, 3400, AXP8191_BLDO1OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 6, 0,
	{ {500, 3400, 100}, } },

	{ "bldo2", 500, 3400, AXP8191_BLDO2OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL1, 7, 0,
	{ {500, 3400, 100}, } },

	{ "bldo3", 500, 3400, AXP8191_BLDO3OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 0, 0,
	{ {500, 3400, 100}, } },

	{ "bldo4", 500, 3400, AXP8191_BLDO4OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 1, 0,
	{ {500, 3400, 100}, } },

	{ "bldo5", 500, 3400, AXP8191_BLDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 2, 0,
	{ {500, 3400, 100}, } },

	{ "cldo1", 500, 3400, AXP8191_CLDO1OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 3, 0,
	{ {500, 3400, 100}, } },

	{ "cldo2", 500, 3400, AXP8191_CLDO2OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 4, 0,
	{ {500, 3400, 100}, } },

	{ "cldo3", 500, 3400, AXP8191_CLDO3OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 5, 0,
	{ {500, 3400, 100}, } },

	{ "cldo4", 500, 3400, AXP8191_CLDO4OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 6, 0,
	{ {500, 3400, 100}, } },

	{ "cldo5", 500, 3400, AXP8191_CLDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL2, 7, 0,
	{ {500, 3400, 100}, } },

	{ "dldo1", 500, 3400, AXP8191_DLDO1OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 0, 0,
	{ {500, 3400, 100}, } },

	{ "dldo2", 500, 3400, AXP8191_DLDO2OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 1, 0,
	{ {500, 3400, 100}, } },

	{ "dldo3", 500, 3400, AXP8191_DLDO3OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 2, 0,
	{ {500, 3400, 100}, } },

	{ "dldo4", 500, 3400, AXP8191_DLDO4OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 3, 0,
	{ {500, 3400, 100}, } },

	{ "dldo5", 500, 3400, AXP8191_DLDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 4, 0,
	{ {500, 3400, 100}, } },

	{ "dldo5", 500, 3400, AXP8191_DLDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 5, 0,
	{ {500, 3400, 100}, } },

	{ "eldo1", 500, 1500, AXP8191_ELDO1OUT_VOL, 0x3f, AXP8191_LDO_POWER_ON_OFF_CTL3, 6, 0,
	{ {500, 1500, 25}, } },

	{ "eldo2", 500, 1500, AXP8191_ELDO2OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 7, 0,
	{ {500, 1500, 100}, } },

	{ "eldo3", 500, 1500, AXP8191_ELDO3OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 0, 0,
	{ {500, 1500, 100}, } },

	{ "eldo4", 500, 1500, AXP8191_ELDO4OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL3, 1, 0,
	{ {500, 1500, 100}, } },

	{ "eldo5", 500, 1500, AXP8191_ELDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL4, 2, 0,
	{ {500, 1500, 100}, } },

	{ "eldo5", 500, 1500, AXP8191_ELDO5OUT_VOL, 0x1f, AXP8191_LDO_POWER_ON_OFF_CTL4, 3, 0,
	{ {500, 1500, 100}, } },
};
/* clang-format on */

int pmu_axp8191_init(sunxi_i2c_t *i2c_dev) {
	uint8_t axp_val;
	int ret;

	if (!i2c_dev->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if (ret = sunxi_i2c_read(i2c_dev, AXP8191_RUNTIME_ADDR, AXP8191_CHIP_ID, &axp_val)) {
		printk_warning("PMU: Probe target device AXP8191 failed. ret = %d\n", ret);
		return -1;
	}

	if (axp_val == AXP8191_IC_TYPE) {
		printk_info("PMU: Found AXP318W PMU, Addr 0x%02x\n", AXP8191_RUNTIME_ADDR);
	} else {
		printk_warning("PMU: AXP PMU Check error\n");
		return -1;
	}

	sunxi_i2c_read(i2c_dev, AXP8191_DCDC_POWER_ON_OFF_CTL1, AXP8191_CHIP_ID, &axp_val);
	axp_val |= 0x08;
	sunxi_i2c_write(i2c_dev, AXP8191_DCDC_POWER_ON_OFF_CTL1, AXP8191_CHIP_ID, axp_val);

	/* enable dcdc2~dcdc9 dvm */
	for (int i = 0; i <= (AXP8191_DC9OUT_VOL - AXP8191_DC2OUT_VOL); i++) {
		sunxi_i2c_read(i2c_dev, AXP8191_RUNTIME_ADDR, AXP8191_DC2OUT_VOL + i, &axp_val);
		axp_val |= 0x80;
		sunxi_i2c_write(i2c_dev, AXP8191_RUNTIME_ADDR, AXP8191_DC2OUT_VOL + i, axp_val);
	}

	return AXP8191_CHIP_ID;
}

int pmu_axp8191_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff) {
	return axp_set_vol(i2c_dev, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP8191_RUNTIME_ADDR);
}

int pmu_axp8191_get_vol(sunxi_i2c_t *i2c_dev, char *name) {
	return axp_get_vol(i2c_dev, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP8191_RUNTIME_ADDR);
}

void pmu_axp8191_dump(sunxi_i2c_t *i2c_dev) {
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) { printk_debug("PMU: axp8191 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp8191_get_vol(i2c_dev, axp_ctrl_tbl[i].name)); }
}