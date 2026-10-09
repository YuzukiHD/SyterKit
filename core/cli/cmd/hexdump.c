/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file hexdump.c
 * @brief Shell command `hexdump`.
 */
#include <stddef.h>
#include <stdlib.h>
#include <log.h>
#include <cli/cli.h>

msh_define_help(hexdump, "dumps memory region in hex",
	"Usage: hexdump [address] [length]\n");

/**
 * @brief Dump a memory range in hexadecimal form.
 *
 * Address and length accept the integer syntax understood by @c strtol,
 * including a hexadecimal @c 0x prefix.  The command validates its arity
 * before touching the requested address range.
 *
 * @param[in] argc Number of command arguments.
 * @param[in] argv Argument vector containing address and length.
 * @return Zero on success, or one when the usage is invalid.
 */
int cmd_hexdump(int argc, const char **argv)
{
	if (argc != 3) {
		printk(LOG_LEVEL_MUTE, "Usage: hexdump [address] [length]\n");
		return 1;
	}

	uint32_t start_addr = strtol(argv[1], NULL, 0);
	uint32_t len = strtol(argv[2], NULL, 0);

	dump_hex(start_addr, len);

	return 0;
}
