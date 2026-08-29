/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "psram-sun252iw2: " fmt

/**
 * @file psram-sun252iw2.c
 * @brief PSRAM initialization glue for the sun252iw2 SoC.
 *
 * Bridges the prebuilt PSRAM library used by the SoC boot firmware to the
 * Sunxi PSRAM framework and supplies the cache and delay helpers the library
 * expects.
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
#include <drivers/psram/psram.h>
#include <drivers/pmu/axp.h>

#include <common.h>

/* Provided by the prebuilt PSRAM init library (boards/<board>/lib/libpsram.a). */
extern int lpsram_init(void *buff);

/**
 * @brief Microsecond delay helper used by the PSRAM library.
 *
 * @param[in] us Delay duration in microseconds.
 */
void __usdelay(unsigned long us)
{
	udelay(us);
}

/**
 * @brief Invalidate the entire data cache.
 */
void csi_l2c_clear_invalid_all(void)
{
	invalidate_dcache_all();
	return;
}

/**
 * @brief Clean (flush) the entire data cache.
 */
void csi_l2c_clear_all(void)
{
	flush_dcache_all();
	return;
}

/**
 * @brief Set the DDR voltage.
 *
 * This platform has no software-controlled DDR supply, so the request is
 * always accepted and reports success without programming any regulator.
 *
 * @param[in] vol_val Requested voltage value in millivolts.
 * @return 0 on success.
 */
int set_ddr_voltage(unsigned int vol_val)
{
	return 0;
}

/**
 * @brief Initialize the PSRAM controller and memory.
 *
 * Delegates to the prebuilt PSRAM library with the parameters supplied in the
 * descriptor and records the reported memory size.
 *
 * @param[in,out] psram PSRAM descriptor carrying the parameter table.
 * @return The initialized PSRAM size in bytes, or zero on failure.
 */
uint32_t sunxi_psram_init(sunxi_psram_t *psram)
{
	if (psram == NULL || psram->parameter_count == 0U)
		return 0U;
	psram->size = lpsram_init((void *)psram->parameters);
	return psram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-psram");
