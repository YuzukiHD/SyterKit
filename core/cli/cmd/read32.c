/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file read32.c
 * @brief Shell command `read32`.
 */
#include <stddef.h>
#include <stdlib.h>
#include <io.h>
#include <log.h>
#include <cli/cli.h>

msh_define_help(read32, "read 32-bits value from device reg",
	"Usage: read32 [address]\n");

/**
 * @brief Read and print one 32-bit memory-mapped register.
 * @param[in] argc Number of command arguments.
 * @param[in] argv Argument vector containing the register address in hex.
 * @return Zero on success, or one when the usage is invalid.
 */
int cmd_read32(int argc, const char **argv)
{
	if (argc != 2) {
		printk(LOG_LEVEL_MUTE, "Usage: read32 [address]\n");
		return 1;
	}

	uint32_t ptr = (uint32_t)simple_strtoul(argv[1], NULL, 16);
	uint32_t value = read32(ptr);

	printk(LOG_LEVEL_MUTE, "Value at address 0x%08x: 0x%08X\n", ptr, value);

	return 0;
}
