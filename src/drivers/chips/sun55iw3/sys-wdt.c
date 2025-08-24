/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <reg-ncat.h>

void sys_reset() {
	write32(SUNXI_WDT_BASE + 0xa8, (0x16aa << 16) | (0x1 << 0));
}