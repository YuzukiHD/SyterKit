/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

void sys_reset() {
	write32(0x020500a0 + 0x08, (0x16aa << 16) | (0x1 << 0));
}