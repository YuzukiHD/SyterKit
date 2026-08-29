/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Avaota A1 (sun55iw3).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun55iw3.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <dt-compatible/gpio-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/sid-dt.h>

/**
 * @brief Wake the GIC redistributor for CPU0.
 *
 * Sets the ProcessorSleep bit in the GICR_WAKER register so the GIC
 * redistributor can deliver interrupts to the boot CPU.
 */
void gicr_set_waker(void)
{
	uint32_t gicr_waker = read32(GICR_WAKER(0));
	if ((gicr_waker & 2) == 0) {
		gicr_waker |= 2;
		write32(GICR_WAKER(0), gicr_waker);
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
 * @brief Set the R_PIO (PL) GPIO power mode according to the detected level.
 *
 * Reads the PL GPIO voltage indicator and programs the power mode register
 * for a 1.8 V rail when the GPIO level indicates 1.8 V operation.
 */
void set_rpio_power_mode(void)
{
	sunxi_gpio_t r_pio;
	uint32_t reg_val;

	if (sunxi_gpio_dt_read_alias(&r_pio, "gpio1") != DRIVER_OK) {
		printk_error("GPIO: invalid R_PIO devicetree configuration\n");
		return;
	}
	reg_val = read32(r_pio.base + 0x348);
	if (reg_val & 0x1) {
		printk_debug("PL gpio voltage : 1.8V \n");
		write32(r_pio.base + 0x340, 0x1);
	} else {
		printk_debug("PL gpio voltage : 3.3V \n");
	}
}

/**
 * @brief Initialize the NSI (NoC) port priorities and autogating.
 *
 * Programs the AXI interconnect priority registers for the main bus masters
 * and disables the RA, TA, and PCIe autogating blocks.
 *
 * @return 0 on success.
 */
int sunxi_nsi_init(void)
{
	/* IOMMU prio 3 */
	writel(0x1, 0x02021418);
	writel(0xf, 0x02021414);
	/* DE prio 2 */
	writel(0x1, 0x02021a18);
	writel(0xa, 0x02021a14);
	/* VE R prio 2 */
	writel(0x1, 0x02021618);
	writel(0xa, 0x02021614);
	/* VE RW prio 2 */
	writel(0x1, 0x02021818);
	writel(0xa, 0x02021814);
	/* ISP prio 2 */
	writel(0x1, 0x02020c18);
	writel(0xa, 0x02020c14);
	/* CSI prio 2 */
	writel(0x1, 0x02021c18);
	writel(0xa, 0x02021c14);
	/* NPU prio 2 */
	writel(0x1, 0x02020a18);
	writel(0xa, 0x02020a14);

	/* close ra0 autogating */
	writel(0x0, 0x02023c00);
	/* close ta autogating */
	writel(0x0, 0x02023e00);
	/* close pcie autogating */
	writel(0x0, 0x02020600);
	return 0;
}

/**
 * @brief Print the SoC identification banner for the Avaota A1 board.
 *
 * Reads the 128-bit chip SID from the eFuses through the devicetree SID
 * alias and prints the board model, CPU cores, chip SID, chip type, and
 * chip version to the console.
 */
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

	printk_info("Model: AvaotaSBC Avaota A1 board.\n");
	printk_info("Core: Arm Octa-Core Cortex-A55 v65 r2p0\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
	case 0x5200:
		printk_info("Chip type = A523M00X0000");
		break;
	case 0x5f10:
		printk_info("Chip type = T527M02X0DCH");
		break;
	case 0x5f30:
		printk_info("Chip type = T527M00X0DCH");
		break;
	case 0x5500:
		printk_info("Chip type = MR527M02X0D00");
		break;
	case 0xff10:
		printk_info("Chip type = A527M000000H");
		break;
	default:
		printk_info("Chip type = UNKNOW");
		break;
	}

	uint32_t version = read32(SUNXI_SYSCTRL_BASE + 0x24) & 0x7;
	printk(LOG_LEVEL_MUTE, " Chip Version = %x \n", version);
}

/**
 * @brief Reset the system using the watchdog.
 *
 * Programs the watchdog with the reset key and then spins forever while the
 * SoC performs the reset.
 */
void sys_reset(void)
{
	write32(SUNXI_WDT_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
