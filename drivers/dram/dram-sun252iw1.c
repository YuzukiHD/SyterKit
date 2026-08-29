/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "dram-sun252iw1: " fmt

/**
 * @file dram-sun252iw1.c
 * @brief DRAM controller driver for the Allwinner sun252iw1 SoC.
 *
 * Provides the microsecond delay, cache maintenance and DDR voltage hooks
 * used by the DRAM init blob, and drives the external DRAM initialization
 * routine.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <dt2c/driver.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/pmu/axp.h>

#include <common.h>

extern int init_DRAM(int type, void *buff);

/**
 * @brief Delay for a number of microseconds.
 *
 * @param[in] us Delay duration in microseconds.
 */
void __usdelay(unsigned long us)
{
	udelay(us);
}

/**
 * @brief Invalidate and clear the L2 cache.
 *
 * Invalidates the entire data cache so stale entries do not survive the DRAM
 * initialization.
 */
void csi_l2c_clear_invalid_all(void)
{
	invalidate_dcache_all();
	return;
}

/**
 * @brief Flush the L2 cache.
 *
 * Flushes the entire data cache so pending writes are visible after the DRAM
 * initialization.
 */
void csi_l2c_clear_all(void)
{
	flush_dcache_all();
	return;
}

/**
 * @brief Set the DDR supply voltage.
 *
 * The voltage is configured by the external DRAM init blob, so this is a
 * no-op on this SoC.
 *
 * @param[in] vol_val Target voltage value in millivolts.
 *
 * @return 0 on success.
 */
int set_ddr_voltage(unsigned int vol_val)
{
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
