/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#define __SYTERKIT__ 1
#define __arm__ 1
#define __thumb__ 1
#define RUAPU_IMPLEMENTATION 1
#define RUAPU_BAREMETAL 1

// Hack for arm-linux-gnueabi
#ifdef __linux__
#undef __linux__
#endif

#include "ruapu.h"

#define PRINT_ISA_SUPPORT(isa) printk_info("%s = %d\n", #isa, ruapu_supports(#isa));

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_serial_init(&uart_dbg);

	show_banner();

	sunxi_clk_init();

	printk_info("Hello World! Now Running RUAPU Test!\n");

	ruapu_init();

	PRINT_ISA_SUPPORT(edsp)
	PRINT_ISA_SUPPORT(neon)
	PRINT_ISA_SUPPORT(vfpv4)
	PRINT_ISA_SUPPORT(idiv)


	printk_info("Ruapu Supported:\n");
	const char *const *supported = ruapu_rua();
	while (*supported) {
		printk_info("%s\n", *supported);
		supported++;
	}

	printk_info("RUAPU Test done!\n");

	return 0;
}