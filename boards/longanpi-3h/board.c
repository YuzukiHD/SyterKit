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
#include <drivers/sdcard.h>
#include <drivers/sid.h>
#include <drivers/spi.h>
#include <drivers/serial.h>

sdhci_t sdhci0 = {
		.name = "sdhci0",
		.id = 0,
		.reg = (sdhci_reg_t *) SUNXI_SMHC0_BASE,
		.voltage = MMC_VDD_27_36,
		.width = MMC_BUS_WIDTH_4,
		.clock = MMC_CLK_50M,
		.removable = 0,
		.isspi = FALSE,
		.skew_auto_mode = FALSE,
		.sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
		.gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
		.gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
		.gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
		.gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
		.gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
		.gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

void neon_enable(void) {
	/* set NSACR, both Secure and Non-secure access are allowed to NEON */
	asm volatile("MRC p15, 0, r0, c1, c1, 2");
	asm volatile("ORR r0, r0, #(0x3<<10) @ enable fpu/neon");
	asm volatile("MCR p15, 0, r0, c1, c1, 2");
	/* Set the CPACR for access to CP10 and CP11*/
	asm volatile("LDR r0, =0xF00000");
	asm volatile("MCR p15, 0, r0, c1, c0, 2");
	/* Set the FPEXC EN bit to enable the FPU */
	asm volatile("MOV r3, #0x40000000");
	/*@VMSR FPEXC, r3*/
	asm volatile("MCR p10, 7, r3, c8, c0, 0");
}

const uint32_t dram_para_ddr3[32] = {
		648,
		3,
		0x03030303,
		0x0e0e0e0e,
		0x1f12,
		1,
		0x30fb,
		0x0000,
		0x840,
		0x4,
		0x8,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0xC0001002,
		0x0,
		0x0,
		0x0,
		0x33808080,
		0x002F1107,
		0xddddcccc,
		0xeddc7665,
		0x40,
};

uint32_t dram_para[32] = {
		0x2d0,
		0x8,
		0xc0c0c0c,
		0xe0e0e0e,
		0xa0e,
		0x7887ffff,
		0x30fa,
		0x4000000,
		0x0,
		0x34,
		0x1b,
		0x33,
		0x3,
		0x0,
		0x0,
		0x4,
		0x72,
		0x0,
		0x9,
		0x0,
		0x0,
		0x24,
		0x0,
		0x0,
		0x0,
		0x0,
		0x39808080,
		0x402f6603,
		0x20262620,
		0xe0e0f0f,
		0x1024,
		0x0,
};

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
	if (((readl(SUNXI_SID_BASE + 0x248) >> 29) & 0x1) == 1) {
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
