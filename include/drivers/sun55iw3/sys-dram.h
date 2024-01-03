/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUN55IW3_SYS_DRAM_H__
#define __SUN55IW3_SYS_DRAM_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#define SDRAM_BASE (0x40000000)

uint64_t sunxi_dram_init(void *para);

#endif // __SUN55IW3_SYS_DRAM_H__
