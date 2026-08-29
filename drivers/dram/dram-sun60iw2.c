/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file dram-sun60iw2.c
 * @brief DRAM controller driver for the Allwinner sun60iw2 SoC.
 *
 * Provides the SMC enable workaround and AXP8191 PMU voltage hooks used by
 * the DRAM init blob, and drives the external DRAM initialization routine.
 */

#include <barrier.h>
#include <io.h>
#include <mmu.h>
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

static axp_pmu_t *dram_pmu_axp8191;

extern int init_DRAM(int type, void *buff);

/**
 * @brief Enable the SMC with the glitch workaround.
 *
 * Placeholder kept for compatibility with the DRAM init blob; no action is
 * needed on this SoC.
 */
void sunxi_smc_en_with_glitch_workaround(void)
{
	return;
}

/**
 * @brief Set the voltage of a named DDR supply rail.
 *
 * Programs the AXP8191 PMU rail identified by name.
 *
 * @param[in] name    PMU rail name.
 * @param[in] set_vol Target voltage value in millivolts.
 * @param[in] on      Non-zero to enable the rail, zero to disable it.
 *
 * @return 0 on success.
 */
int set_ddr_voltage_ext(char *name, int set_vol, int on)
{
	printk_debug("PMU: %s set vol %d, onoff %d\n", name, set_vol, on);
	pmu_axp8191_set_vol(dram_pmu_axp8191, name, set_vol, on);
	return 0;
}

/**
 * @brief Initialize the DRAM controller.
 *
 * Validates the DRAM parameters, saves the AXP8191 PMU handle and delegates
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
	dram_pmu_axp8191 = dram->pmu;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
