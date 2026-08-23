/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <drivers/clk/sun300iw1/reg.h>
#include <dt-bindings/soc/sun300iw1.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/sid-dt.h>
#include <e907/sysmap.h>

void show_chip() {
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_sid_read_sram(&sid, 0x0U);
	chip_sid[1] = sunxi_sid_read_sram(&sid, 0x4U);
	chip_sid[2] = sunxi_sid_read_sram(&sid, 0x8U);
	chip_sid[3] = sunxi_sid_read_sram(&sid, 0xcU);

	printk_info("Model: AvaotaSBC Avaota F1 board.\n");
	printk_info("Core: XuanTie E907 RISC-V Core.\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

int sunxi_hosc_detect(void) {
	uintptr_t detect_reg;
	uint32_t val;


	detect_reg = SUNXI_CCU_AON_BASE + HOSC_FREQ_DET;
	val = readl(detect_reg);

	writel(val & (~HOSC_FREQ_DET_HOSC_CLEAR_MASK), detect_reg);
	writel(val | HOSC_FREQ_DET_HOSC_ENABLE_DETECT, detect_reg);

	while (!(HOSC_FREQ_DET_HOSC_FREQ_READY_CLEAR_MASK & readl(detect_reg)))
		;

	val = (readl(detect_reg) & HOSC_FREQ_DET_HOSC_FREQ_DET_CLEAR_MASK) >> HOSC_FREQ_DET_HOSC_FREQ_DET_OFFSET;
	if (val < ((HOSC_24M_COUNTER + HOSC_40M_COUNTER) / 2)) {
		current_hosc_freq = HOSC_FREQ_24M;
		return HOSC_FREQ_24M;
	} else {
		current_hosc_freq = HOSC_FREQ_40M;
		return HOSC_FREQ_40M;
	}
}

void sysmap_init(void) {
	sysmap_add_mem_region(0x00000000, 0x10000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x10000000, 0x02000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x12000000, 0x1E000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x30000000, 0x10000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x40000000, 0x28000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x68000000, 0x01000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x69000000, 0x17000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x80000000, 0x7FFFFFFF, SYSMAP_MEM_ATTR_RAM);
}

void sys_reset(void) {
	setbits_le32(SUNXI_PRCM_BASE + 0x1c, 1U << 3);
	write32(SUNXI_RTC_WDG_BASE + 0x18, 0x16aa0000U);
	write32(SUNXI_RTC_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
