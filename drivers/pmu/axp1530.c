/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "axp1530: " fmt

/**
 * @file axp1530.c
 * @brief X-Powers AXP1530 PMU driver.
 *
 * Implements the AXP1530 family PMU probe, dual-phase configuration,
 * voltage read/write and debug dump operations used by the common PMU
 * framework.
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
 * @brief Voltage regulator control table for the AXP1530 PMU.
 */
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

/**
 * @brief Configure the AXP1530 PMU identity and I2C address.
 *
 * @param[in] pmu PMU descriptor to fill in.
 * @param[in] i2c Initialized I2C bus used to reach the PMU.
 * @return DRIVER_OK on success, or DRIVER_ERROR_INVALID on bad arguments.
 */
int pmu_axp1530_config(axp_pmu_t *pmu, sunxi_i2c_t *i2c)
{
	return sunxi_pmu_config(pmu, i2c, AXP_PMU_AXP1530, AXP1530_RUNTIME_ADDR, 0U);
}

/**
 * @brief Probe and initialize the AXP1530 PMU.
 *
 * Identifies the exact PMU variant from the chip ID and enables the over
 * temperature shutdown function.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @return 0 on success, or -1 on probe or register failure.
 */
int pmu_axp1530_init(axp_pmu_t *pmu)
{
	uint8_t axp_val;
	int ret;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP1530) || !pmu->i2c->status) {
		pr_warn("not init\n");
		return -1;
	}

	if ((ret = sunxi_i2c_read(pmu->i2c, pmu->address, AXP1530_VERSION, &axp_val))) {
		pr_warn("Probe target device AXP1530 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	switch (axp_val) {
	case AXP1530_CHIP_ID:
		pr_info("Found AXP1530 PMU\n");
		break;
	case AXP313A_CHIP_ID:
		pr_info("Found AXP313A PMU\n");
		break;
	case AXP313B_CHIP_ID:
		pr_info("Found AXP313B PMU\n");
		break;
	case AXP323_CHIP_ID:
		pr_info("Found AXP323 PMU\n");
		break;
	default:
		pr_info("Cannot found match PMU\n");
		return -1;
	}

	/* Set over temperature shutdown functtion */
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP1530_POWER_DOMN_SEQUENCE, &axp_val))
		return -1;
	axp_val |= (0x1 << 1);
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP1530_POWER_DOMN_SEQUENCE, axp_val))
		return -1;

	return 0;
}

/**
 * @brief Enable dual-phase operation on supported AXP1530 variants.
 *
 * Only the AXP323 variant supports dual-phase operation; other variants
 * are rejected.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @return 0 on success, or -1 when unsupported or on register failure.
 */
int pmu_axp1530_set_dual_phase(axp_pmu_t *pmu)
{
	uint8_t axp_val;
	int ret;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP1530))
		return -1;
	if ((ret = sunxi_i2c_read(pmu->i2c, pmu->address, AXP1530_VERSION, &axp_val))) {
		pr_warn("Probe target device AXP1530 failed. ret = %d\n", ret);
		return -1;
	}

	axp_val &= 0xCF;
	switch (axp_val) {
	case AXP323_CHIP_ID: /* Only AXP323 Support Dual phase */
		break;
	default:
		pr_info("not support dual phase\n");
		return -1;
	}

	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP1530_OUTPUT_MONITOR_CONTROL, 0x1E) || sunxi_i2c_write(pmu->i2c, pmu->address, AXP1530_DCDC_MODE_CTRL2, 0x02) ||
	    sunxi_i2c_write(pmu->i2c, pmu->address, AXP1530_POWER_DOMN_SEQUENCE, 0x22))
		return -1;

	return 0;
}

/**
 * @brief Set the output voltage of a named AXP1530 regulator.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @param[in] name Name of the regulator rail to configure.
 * @param[in] set_vol Desired voltage in millivolts.
 * @param[in] onoff Non-zero to enable the rail, zero to disable it.
 * @return 0 on success, or -1 on error.
 */
int pmu_axp1530_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff)
{
	return axp_set_vol(pmu, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

/**
 * @brief Read the current output voltage of a named AXP1530 regulator.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 * @param[in] name Name of the regulator rail to read.
 * @return The voltage in millivolts, or a negative value on error.
 */
int pmu_axp1530_get_vol(axp_pmu_t *pmu, char *name)
{
	return axp_get_vol(pmu, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl));
}

/**
 * @brief Dump all AXP1530 regulator voltages to the debug log.
 *
 * @param[in] pmu PMU descriptor with the I2C bus and address configured.
 */
void pmu_axp1530_dump(axp_pmu_t *pmu)
{
	for (int i = 0; i < ARRAY_SIZE(axp_ctrl_tbl); i++) {
		pr_debug("AXP1530 %s = %dmv\n", axp_ctrl_tbl[i].name, pmu_axp1530_get_vol(pmu, axp_ctrl_tbl[i].name));
	}
}
