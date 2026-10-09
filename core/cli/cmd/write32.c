/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file write32.c
 * @brief Shell command `write32`.
 */
#include <stddef.h>
#include <stdlib.h>
#include <io.h>
#include <log.h>
#include <cli/cli.h>

msh_define_help(write32, "write 32-bits value to device reg",
	"Usage: write32 [address] [data]\n");

/**
 * @brief Write one 32-bit value to a memory-mapped register.
 * @param[in] argc Number of command arguments.
 * @param[in] argv Argument vector containing address and value in hex.
 * @return Zero on success, or -1 when the usage is invalid.
 */
int cmd_write32(int argc, const char **argv)
{
	if (argc < 3) {
		printk(LOG_LEVEL_MUTE, "Usage: write32 [address] [data]\n");
		return -1;
	}
	uint32_t addr = (uint32_t)simple_strtoul(argv[1], NULL, 16);
	uint32_t data = (uint32_t)simple_strtoul(argv[2], NULL, 16);
	write32(addr, data);
	printk(LOG_LEVEL_MUTE, "Wrote 0x%08x to address 0x%08x\n", data, addr);
	return 0;
}
