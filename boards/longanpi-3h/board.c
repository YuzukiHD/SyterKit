/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Longan Pi 3H (sun50iw9).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun50iw9.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/sid-dt.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

/**
 * @brief Power down a secondary CPU core.
 *
 * Clears the debug and reset control bits, then sets the cluster power-off
 * gating bit for the given CPU core.
 *
 * @param[in] cpu Core number to power down.
 */
void set_cpu_down(unsigned int cpu)
{
	clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_DBG_REG1, 1 << cpu);
	udelay(10);

	setbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CLUSTER_PWROFF_GATING, 1 << cpu);
	udelay(20);

	clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CPU_RST_CTRL, 1 << cpu);
	udelay(10);

	printk_debug("CPU: Power-down cpu-%d ok.\n", cpu);
}

/**
 * @brief Power off the secondary CPU cores when enabled by the eFuses.
 *
 * Reads a SID eFuse flag and, if set, powers down CPU2 and CPU3 to reduce
 * power consumption before OS handoff.
 */
void set_cpu_poweroff(void)
{
	sunxi_sid_t sid;

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	if (((sunxi_efuse_sram_read(&sid, 0x48U) >> 29) & 0x1U) == 1U) {
		set_cpu_down(2); /*power of cpu2*/
		set_cpu_down(3); /*power of cpu3*/
	}
}

/**
 * @brief Disable the MMU, caches, and interrupts before OS handoff.
 */
void clean_syterkit_data(void)
{
	/* Disable MMU, data cache, instruction cache, interrupts */
	arm32_mmu_disable();
	printk_info("disable mmu ok...\n");
	arm32_dcache_disable();
	printk_info("disable dcache ok...\n");
	arm32_icache_disable();
	printk_info("disable icache ok...\n");
	arm32_interrupt_disable();
	printk_info("free interrupt ok...\n");
}

/**
 * @brief Reset the system using the watchdog.
 *
 * Programs the watchdog with the reset key and then spins forever while the
 * SoC performs the reset.
 */
void sys_reset(void)
{
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
