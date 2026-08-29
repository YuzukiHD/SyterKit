/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file dram-sun55iw3.c
 * @brief DRAM controller driver for the Allwinner sun55iw3 SoC.
 *
 * Provides the DDR voltage hook used by the DRAM init blob and drives the
 * external DRAM initialization routine.
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

extern int init_DRAM(int type, void *buff);

/**
 * @brief Set the DDR supply voltage.
 *
 * The DDR voltage is already configured by the main loader, so this only logs
 * the requested value.
 *
 * @param[in] vol_val Target voltage value in millivolts.
 *
 * @return 0 on success.
 */
int set_ddr_voltage(unsigned int vol_val)
{
	/* ddr voltage already set in main */
	printk_info("set_ddr_voltage: %d\n", vol_val);
	return 0;
}

/**
 * @brief Initialize the DRAM controller.
 *
 * Validates the DRAM parameters and delegates to the external init_DRAM
 * routine.
 *
 * @param[in] dram DRAM configuration block.
 *
 * @return Detected DRAM size in bytes, or 0 on failure.
 */
uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
