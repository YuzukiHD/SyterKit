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

#include <drivers/i2c/i2c.h>
#include <drivers/soc/sid.h>
#include <dt-compatible/sid-dt.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

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

void show_chip()
{
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_efuse_sram_read(&sid, 0x0U);
	chip_sid[1] = sunxi_efuse_sram_read(&sid, 0x4U);
	chip_sid[2] = sunxi_efuse_sram_read(&sid, 0x8U);
	chip_sid[3] = sunxi_efuse_sram_read(&sid, 0xcU);

	printk_info("Model: Yuzuki Home Kit\n");
	printk_info("Host Core: Arm Dual-Core Cortex-A7 R2P0\n");
	printk_info("AMP Core: Xuantie C906 RISC-V RV64IMAFDCVX R1S0P2 Vlen=128\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
	case 0x7200:
		printk_info("Chip type = T113M4020DC0");
		break;
	default:
		printk_info("Chip type = UNKNOW");
		break;
	}

	uint32_t version = read32(SUNXI_SYSCRL_BASE + 0x24) & 0x7;
	printk(LOG_LEVEL_MUTE, " Chip Version = %x \n", version);
}

void sys_reset(void)
{
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
