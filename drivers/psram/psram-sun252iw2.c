/* SPDX-License-Identifier: GPL-2.0+ */

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

void __usdelay(unsigned long us)
{
	udelay(us);
}

void csi_l2c_clear_invalid_all(void)
{
	invalidate_dcache_all();
	return;
}

void csi_l2c_clear_all(void)
{
	flush_dcache_all();
	return;
}

int set_ddr_voltage(unsigned int vol_val)
{
	return 0;
}

uint32_t sunxi_psram_init(sunxi_psram_t *psram)
{
	if (psram == NULL || psram->parameter_count == 0U)
		return 0U;
	psram->size = lpsram_init((void *)psram->parameters);
	return psram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-psram");
