/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

uint64_t __attribute__((weak)) sunxi_get_dram_size() {
	return 0;
}

uint32_t __attribute__((weak)) sunxi_dram_init(void *para) {
	return 0;
}