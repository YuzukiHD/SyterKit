/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file dram-sun50iw10.c
 * @brief DRAM controller driver for the Allwinner sun50iw10 SoC.
 *
 * Provides the DDR and DDR4 2.5 V voltage hooks used by the DRAM init blob and
 * drives the external DRAM initialization routine.
 */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <dt2c/driver.h>
#include <drivers/dram/dram.h>
#include <drivers/rtc/rtc.h>
#include <drivers/pmu/axp.h>

static axp_pmu_t *dram_pmu_axp2202;

extern int init_DRAM(int type, void *buff);

/**
 * @brief Set the DDR supply voltage.
 *
 * Programs the AXP2202 dcdc3 rail used for DDR power.
 *
 * @param[in] set_vol Target voltage value in millivolts.
 *
 * @return 0 on success.
 */
int set_ddr_voltage(int set_vol)
{
	printk_info("Set DRAM Voltage to %dmv\n", set_vol);
	if (dram_pmu_axp2202 != NULL)
		pmu_axp2202_set_vol(dram_pmu_axp2202, "dcdc3", set_vol, 1);
	return 0;
}

/**
 * @brief Set the DDR4 2.5 V supply voltage.
 *
 * The DDR4 2.5 V rail is configured externally, so this only logs the
 * requested value.
 *
 * @param[in] set_vol Target voltage value in millivolts.
 *
 * @return 0 on success.
 */
int set_ddr4_2v5_voltage(int set_vol)
{
	printk_info("Set DDR4 25 DRAM Voltage to %dmv\n", set_vol);
	return 0;
}

/**
 * @brief Initialize the DRAM controller.
 *
 * Validates the DRAM parameters, saves the AXP2202 PMU handle and delegates
 * to the external init_DRAM routine.
 *
 * @param[in] dram DRAM configuration block.
 *
 * @return Detected DRAM size in bytes, or 0 on failure.
 */
uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram_pmu_axp2202 = dram->pmu;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
