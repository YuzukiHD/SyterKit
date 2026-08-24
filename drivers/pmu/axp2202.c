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
	{ "dcdc1", 500, 1540, AXP2202_DC1OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 0, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc2", 500, 3400, AXP2202_DC2OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 1, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1600, 3400, 100},} },

	{ "dcdc3", 500, 1840, AXP2202_DC3OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 2, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, } },

	{ "dcdc4", 1000, 3400, AXP2202_DC4OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 3, 0,
	{ {1000, 3400, 100}, } },

	{ "aldo1", 500, 3500, AXP2202_ALDO1OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 0, 0,
	{ {500, 3500, 100}, } },

	{ "aldo2", 500, 3500, AXP2202_ALDO2OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 1, 0,
	{ {500, 3500, 100}, } },

	{ "aldo3", 500, 3500, AXP2202_ALDO3OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 2, 0,
	{ {500, 3500, 100}, } },

	{ "aldo4", 500, 3500, AXP2202_ALDO4OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 3, 0,
	{ {500, 3500, 100}, } },

	{ "bldo1", 500, 3500, AXP2202_BLDO1OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 4, 0,
	{ {500, 3500, 100}, } },

	{ "bldo2", 500, 3500, AXP2202_BLDO2OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 5, 0,
	{ {500, 3500, 100}, } },

	{ "bldo3", 500, 3500, AXP2202_BLDO3OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 6, 0,
	{ {500, 3500, 100}, } },

	{ "bldo4", 500, 3500, AXP2202_BLDO4OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 7, 0,
	{ {500, 3500, 100}, } },

	{ "cldo1", 500, 3500, AXP2202_CLDO1OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 0, 0,
	{ {500, 3500, 100}, } },

	{ "cldo2", 500, 3500, AXP2202_CLDO2OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 1, 0,
	{ {500, 3500, 100}, } },

	{ "cldo3", 500, 3500, AXP2202_CLDO3OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 2, 0,
	{ {500, 3500, 100}, } },

	{ "cldo4", 500, 3500, AXP2202_CLDO4OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 3, 0,
	{ {500, 3500, 100}, } },

	{ "cpusldo", 500, 1400, AXP2202_CPUSLDO_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 4, 0,
	{ {500, 1400, 50}, } },
};
/* clang-format on */

int pmu_axp2202_config(axp_pmu_t *pmu, sunxi_i2c_t *i2c)
{
	return sunxi_pmu_config(pmu, i2c, AXP_PMU_AXP2202, AXP2202_B_RUNTIME_ADDR, AXP2202_C_RUNTIME_ADDR);
}

int pmu_axp2202_init(axp_pmu_t *pmu)
{
	uint8_t axp_val;
	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202) || !pmu->i2c->status) {
		printk_warning("PMU: I2C not init\n");
		return -1;
	}

	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_CHIP_ID_EXT, &axp_val)) {
		if (pmu->fallback_address == 0U || sunxi_i2c_read(pmu->i2c, pmu->fallback_address, AXP2202_CHIP_ID_EXT, &axp_val)) {
			printk_warning("PMU: AXP2202 PMU Read error\n");
			return -1;
		}
		pmu->address = pmu->fallback_address;
	}

	if (axp_val != 0x02) {
		printk_warning("PMU: AXP PMU Check error\n");
		return -1;
	} else {
		printk_info("PMU: Found AXP717 PMU, Addr 0x%02x\n", pmu->address);
	}

	/* limit run current to 2A */
	axp_val = 0x26;
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_VBUS_CUR_SET, axp_val))
		return -1;

	/* set adc channel0 enable */
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_ADC_CH0, &axp_val))
		return -1;
	axp_val |= 0x33;
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_ADC_CH0, axp_val))
		return -1;

	/*pmu set vsys min*/
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_VSYS_MIN, &axp_val))
		return -1;
	axp_val = 0x06;
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_VSYS_MIN, axp_val))
		return -1;

	/*pmu dcdc1 uvp disable */
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_DCDC_PWEOFF_EN, &axp_val))
		return -1;
	axp_val &= ~(1 << 0);
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_DCDC_PWEOFF_EN, axp_val))
		return -1;

	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_CHIP_VER_EXT, &axp_val))
		return -1;

	if (axp_val) {
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_MODULE_EN, &axp_val))
			return -1;
		axp_val |= 0x10;
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_MODULE_EN, axp_val))
			return -1;
	} else {
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_MODULE_EN, &axp_val))
			return -1;
		axp_val &= 0xEF;
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_MODULE_EN, axp_val))
			return -1;
	}

	return 0;
}

int pmu_axp2202_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff)
{
	return axp_set_vol(pmu, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

int pmu_axp2202_get_vol(axp_pmu_t *pmu, char *name)
{
	return axp_get_vol(pmu, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

void pmu_axp2202_dump(axp_pmu_t *pmu)
{
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) {
		printk_debug("PMU: AXP2202 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp2202_get_vol(pmu, axp_ctrl_tbl[i].name));
	}
}
