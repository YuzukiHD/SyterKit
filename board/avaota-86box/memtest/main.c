/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include "sys-dram.h"
#include "sys-rtc.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include "memtester.c"

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

/* 
 * main function for the bootloader. Initializes and sets up the system, loads the kernel and device tree binary from
 * an SD card, sets boot arguments, and boots the kernel. If the kernel fails to boot, the function jumps to FEL mode.
 */
int main(void) {
	/* Initialize the debug serial interface. */
	sunxi_serial_init(&uart_dbg);

	/* Display the bootloader banner. */
	show_banner();

	/* Initialize the system clock. */
	sunxi_clk_init();

	/* Initialize the DRAM and enable memory management unit (MMU). */
	uint32_t dram_size = sunxi_dram_init(&dram_para);
	arm32_mmu_enable(SDRAM_BASE, dram_size);

	/* Debug message to indicate that MMU is enabled. */
	printk_debug("enable mmu ok\n");

	/* Dump information about the system clocks. */
	sunxi_clk_dump();

#define DRAM_TEST_SIZE 32 * 1024 * 1024
#define DRAM_SIZE_BYTE dram_size * 1024 * 1024

	static int i = 0;
	while (1) {
		do_memtester((uint64_t) SDRAM_BASE, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		i++;
	}

_shell:
	syterkit_shell_attach(NULL);

_fel:
	/* If the kernel boot fails, jump to FEL mode. */
	jmp_to_fel();

	/* Return 0 to indicate successful execution. */
	return 0;
}
