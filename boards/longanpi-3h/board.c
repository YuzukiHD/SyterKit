/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <drivers/reg/reg-ncat.h>
#include <drivers/clk.h>

#include <mmu.h>

#include <drivers/dram.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/sid.h>
#include <dt-compatible/sid-dt.h>
#include <drivers/spi.h>
#include <drivers/serial.h>

void set_cpu_down(unsigned int cpu) {
	clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_DBG_REG1, 1 << cpu);
	udelay(10);

	setbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CLUSTER_PWROFF_GATING, 1 << cpu);
	udelay(20);

	clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CPU_RST_CTRL, 1 << cpu);
	udelay(10);

	printk_debug("CPU: Power-down cpu-%d ok.\n", cpu);
}

void set_cpu_poweroff(void) {
	sunxi_sid_t sid;

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	if (((sunxi_sid_read_sram(&sid, 0x48U) >> 29) & 0x1U) == 1U) {
		set_cpu_down(2); /*power of cpu2*/
		set_cpu_down(3); /*power of cpu3*/
	}
}

void clean_syterkit_data(void) {
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

void sys_reset(void) {
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
