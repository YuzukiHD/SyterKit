/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "axp2101: " fmt

/**
 * @file axp2101.c
 * @brief X-Powers AXP2101 PMU driver.
 *
 * Implements the AXP2101 PMU probe, voltage read/write and debug dump
 * operations used by the common PMU framework.
 */

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
/**
 * @brief Voltage regulator control table for the AXP2101 PMU.
 */
static axp_contrl_info axp2101_ctrl_tbl[] = {
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

/**
 * @brief Configure the AXP2101 PMU identity and I2C address.
 *
 * @param[in] pmu PMU descriptor to fill in.
 * @param[in] i2c Initialized I2C bus used to reach the PMU.
 * @return DRIVER_OK on success, or DRIVER_ERROR_INVALID on bad arguments.
 */
int pmu_axp2101_config(axp_pmu_t *pmu, sunxi_i2c_t *i2c)
{
	return sunxi_pmu_config(pmu, i2c, AXP_PMU_AXP2101, AXP2101_RUNTIME_ADDR, 0U);
}

/**
 * @brief Probe and initialize the AXP2101 PMU.
 *
 * Verifies the chip ID over I2C and applies the charge current limit, VBUS
 * current limit, ADC, PWM mode, VSYS minimum and power control settings.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @return AXP2101_CHIP_ID on success, or -1 on probe or register failure.
 */
int pmu_axp2101_init(axp_pmu_t *pmu)
{
	uint8_t axp_val;
	uint8_t reg_value;
	int ret;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2101) || !pmu->i2c->status) {
		pr_warn("not init\n");
		return -1;
	}

	if ((ret = sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_VERSION, &axp_val))) {
		pr_warn("Probe target device AXP2101 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	if (axp_val == AXP2101_CHIP_ID || axp_val == AXP2101_CHIP_ID_B) {
		pr_info("Found AXP2101 PMU\n");

		/* limit charge current to 300mA */
		reg_value = 0x9;
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_CHARGE1, reg_value))
			return -1;

		/* limit run current to 2A */
		reg_value = 0x5;
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_VBUS_CUR_SET, reg_value))
			return -1;

		/* enable vbus adc channel */
		if (axp_val != AXP2101_CHIP_ID_B) {
			reg_value = 0x40;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_BAT_AVERVOL_H6, reg_value))
				return -1;
		}

		/* set dcdc1 & dcdc3 & dcdc2  & dcdc4 pwm mode */
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_OUTPUT_CTL1, &reg_value))
			return -1;
		reg_value |= ((1 << 2) | (1 << 4) | (1 << 3) | (1 << 5));
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_OUTPUT_CTL1, reg_value))
			return -1;

		/* pmu disable soften3 signal */
		if (axp_val != AXP2101_CHIP_ID_B) {
			reg_value = 0x00;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_TWI_ADDR_EXT, reg_value))
				return -1;
			reg_value = 0x06;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_EFUS_OP_CFG, reg_value))
				return -1;
			reg_value = 0x04;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_EFREQ_CTRL, reg_value))
				return -1;
			reg_value = 0x01;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_TWI_ADDR_EXT, reg_value))
				return -1;
			reg_value = 0x30;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_SELLP_CFG, reg_value))
				return -1;
			reg_value = 0x00;
			if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_TWI_ADDR_EXT, reg_value) || sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_EFREQ_CTRL, reg_value) ||
			    sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_EFUS_OP_CFG, reg_value))
				return -1;
		}

		/* pmu set vsys min */
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_VSYS_MIN, &reg_value))
			return -1;
		reg_value &= ~(0x7 << 4);
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_VSYS_MIN, reg_value))
			return -1;

		/* pmu set vimdpm cfg */
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_VBUS_VOL_SET, &reg_value))
			return -1;
		reg_value &= ~(0xf << 0);
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_VBUS_VOL_SET, reg_value))
			return -1;

		/* pmu reset enable */
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_OFF_CTL, &reg_value))
			return -1;
		reg_value |= (3 << 2);
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_OFF_CTL, reg_value))
			return -1;

		/* pmu pwroff enable */
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_PWEON_PWEOFF_EN, &reg_value))
			return -1;
		reg_value |= (1 << 1);
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_PWEON_PWEOFF_EN, reg_value))
			return -1;

		/* pmu dcdc1 pwroff enable */
		if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2101_DCDC_PWEOFF_EN, &reg_value))
			return -1;
		reg_value &= ~(1 << 0);
		if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2101_DCDC_PWEOFF_EN, reg_value))
			return -1;

		return AXP2101_CHIP_ID;
	}
	return -1;
}

/**
 * @brief Set the output voltage of a named AXP2101 regulator.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @param[in] name Name of the regulator rail to configure.
 * @param[in] set_vol Desired voltage in millivolts.
 * @param[in] onoff Non-zero to enable the rail, zero to disable it.
 * @return 0 on success, or -1 on error.
 */
int pmu_axp2101_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff)
{
	return axp_set_vol(pmu, name, set_vol, onoff, axp2101_ctrl_tbl, ARRAY_SIZE(axp2101_ctrl_tbl));
}

/**
 * @brief Read the current output voltage of a named AXP2101 regulator.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @param[in] name Name of the regulator rail to read.
 * @return The voltage in millivolts, or a negative value on error.
 */
int pmu_axp2101_get_vol(axp_pmu_t *pmu, char *name)
{
	return axp_get_vol(pmu, name, axp2101_ctrl_tbl, ARRAY_SIZE(axp2101_ctrl_tbl));
}

/**
 * @brief Dump all AXP2101 regulator voltages to the debug log.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 */
void pmu_axp2101_dump(axp_pmu_t *pmu)
{
	for (int i = 0; i < ARRAY_SIZE(axp2101_ctrl_tbl); i++) {
		pr_debug("AXP2101 %s = %dmv\n", axp2101_ctrl_tbl[i].name, pmu_axp2101_get_vol(pmu, axp2101_ctrl_tbl[i].name));
	}
}
