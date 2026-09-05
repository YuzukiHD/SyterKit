/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun8iw20.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

void clean_syterkit_data(void)
{
	/* Disable MMU, data cache, instruction cache, interrupts */
	arm32_mmu_disable();
	pr_info("disable mmu ok...\n");
	arm32_dcache_disable();
	pr_info("disable dcache ok...\n");
	arm32_icache_disable();
	pr_info("disable icache ok...\n");
	arm32_interrupt_disable();
	pr_info("free interrupt ok...\n");
}

void sys_reset(void)
{
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
