/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_EFEX_H__
#define __SYTERKIT_EFEX_H__

#include <stddef.h>
#include <stdint.h>

#define SYTERKIT_EFEX_DRAM_MAGIC 0x4d415244U
#define SYTERKIT_EFEX_DRAM_PARA_WORDS 32U

struct syterkit_efex_result {
	uint32_t dram_init_flag;
	uint32_t dram_update_flag;
	uint32_t dram_paras[SYTERKIT_EFEX_DRAM_PARA_WORDS];
};

extern struct syterkit_efex_result syterkit_efex_result;

void syterkit_efex_set_dram_result(const uint32_t *parameters, size_t count);

#endif
