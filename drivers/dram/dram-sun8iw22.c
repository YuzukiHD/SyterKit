/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "dram-sun8iw22: " fmt

/**
 * @file dram-sun8iw22.c
 * @brief DRAM controller driver for the Allwinner sun8iw22 SoC.
 *
 * Provides the microsecond delay and DDR voltage hooks used by the DRAM init
 * blob, and drives the external DRAM initialization routine.
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
 * @brief Delay for a number of microseconds.
 *
 * @param[in] us Delay duration in microseconds.
 */
void __usdelay(unsigned long us)
{
	udelay(us);
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
