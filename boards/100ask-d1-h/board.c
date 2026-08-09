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
#include <drivers/sdcard.h>
#include <drivers/spi.h>
#include <drivers/serial.h>

sdhci_t sdhci0 = {
		.name = "sdhci0",
		.id = 0,
		.reg = (sdhci_reg_t *) 0x04020000,
		.voltage = MMC_VDD_27_36,
		.width = MMC_BUS_WIDTH_4,
		.clock = MMC_CLK_50M,
		.removable = 0,
		.isspi = FALSE,
		.skew_auto_mode = TRUE,
		.sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
		.gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
		.gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
		.gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
		.gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
		.gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
		.gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

dram_para_t dram_para = {
		.dram_clk = 792,
		.dram_type = 3,
		.dram_zq = 0x7b7bfb,
		.dram_odt_en = 0x01,
		.dram_para1 = 0x000010d2,
		.dram_para2 = 0,
		.dram_mr0 = 0x1c70,
		.dram_mr1 = 0x42,
		.dram_mr2 = 0x18,
		.dram_mr3 = 0,
		.dram_tpr0 = 0x004a2195,
		.dram_tpr1 = 0x02423190,
		.dram_tpr2 = 0x0008b061,
		.dram_tpr3 = 0xb4787896,// unused
		.dram_tpr4 = 0,
		.dram_tpr5 = 0x48484848,
		.dram_tpr6 = 0x00000048,
		.dram_tpr7 = 0x1620121e,// unused
		.dram_tpr8 = 0,
		.dram_tpr9 = 0,// clock?
		.dram_tpr10 = 0,
		.dram_tpr11 = 0x00770000,
		.dram_tpr12 = 0x00000002,
		.dram_tpr13 = 0x34050100,
};

void clean_syterkit_data(void) {
}

void sys_reset(void) {
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
