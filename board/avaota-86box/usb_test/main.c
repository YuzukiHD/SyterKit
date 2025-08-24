/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include <log.h>

#include <sys-dram.h>
#include <sys-rtc.h>
#include <sys-sdcard.h>
#include <usb.h>

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

void arm32_do_irq(struct arm_regs_t *regs) {
	do_irq(regs);
}

int main(void) {
	sunxi_serial_init(&uart_dbg);

	/* Display the bootloader banner. */
	show_banner();

	sunxi_clk_init();

	printk_info("Hello World!\n");

	/* Initialize the DRAM and enable memory management unit (MMU). */
	uint32_t dram_size = sunxi_dram_init(&dram_para);
	arm32_mmu_enable(SDRAM_BASE, dram_size);

	/* Debug message to indicate that MMU is enabled. */
	printk_debug("enable mmu ok\n");

	/* Initialize the small memory allocator. */
	smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	/* Dump information about the system clocks. */
	sunxi_clk_dump();

	sunxi_usb_attach_module(SUNXI_USB_DEVICE_MASS);

	if (sunxi_usb_init()) {
		printk_info("USB init failed.\n");
	}

	printk_info("USB init OK.\n");

	sunxi_usb_attach();

	abort();

	return 0;
}