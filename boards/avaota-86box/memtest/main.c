/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <dt-compatible/ccu-dt.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <drivers/dram/dram.h>
#include <drivers/rtc/rtc.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dram-dt.h>

#include "memtester.c"

extern sunxi_serial_t uart_dbg;

static sunxi_dram_t dram;

/* 
 * main function for the bootloader. Initializes and sets up the system, loads the kernel and device tree binary from
 * an SD card, sets boot arguments, and boots the kernel. If the kernel fails to boot, the function jumps to FEL mode.
 */
int main(void) {
	sunxi_ccu_t ccu;
	/* Initialize the debug serial interface. */

	/* Display the bootloader banner. */
	show_banner();

	/* Initialize the system clock. */
	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	/* Initialize the DRAM and enable memory management unit (MMU). */
	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);
	arm32_mmu_enable(dram.memory_base, dram_size);

	/* Debug message to indicate that MMU is enabled. */
	printk_debug("enable mmu ok\n");

	/* Dump information about the system clocks. */
	sunxi_clk_dump(&ccu);

#define DRAM_TEST_SIZE 32 * 1024 * 1024
#define DRAM_SIZE_BYTE dram_size * 1024 * 1024

	static int i = 0;
	while (1) {
		do_memtester((uint64_t) dram.memory_base, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		i++;
	}

	syterkit_shell_attach(NULL);

	/* If the kernel boot fails, jump to FEL mode. */
	jmp_to_fel();

	/* Return 0 to indicate successful execution. */
	return 0;
}
