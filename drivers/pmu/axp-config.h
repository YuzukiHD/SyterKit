/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file axp-config.h
 * @brief Shared PMU configuration helper for the X-Powers AXP PMU drivers.
 *
 * Provides the sunxi_pmu_config() helper used by the per-chip AXP driver
 * files to fill in the common PMU identity and I2C runtime address fields.
 */

#ifndef __DRIVERS_PMU_AXP_CONFIG_H__
#define __DRIVERS_PMU_AXP_CONFIG_H__

#include <driver.h>
#include <drivers/pmu/axp.h>

/* PMU identity and runtime addresses belong to the selected chip driver. */
static inline int sunxi_pmu_config(axp_pmu_t *pmu, sunxi_i2c_t *i2c, axp_pmu_type_t type, uint8_t address, uint8_t fallback_address)
{
	if (pmu == NULL || i2c == NULL || address == 0U || fallback_address == address || fallback_address > 0x7fU)
		return DRIVER_ERROR_INVALID;
	*pmu = (axp_pmu_t){
		.i2c = i2c,
		.address = address,
		.fallback_address = fallback_address,
		.type = type,
	};
	return DRIVER_OK;
}

#endif /* __DRIVERS_PMU_AXP_CONFIG_H__ */
