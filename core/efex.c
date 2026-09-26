/* SPDX-License-Identifier: GPL-2.0+ */

#include <efex.h>

#include <cache.h>

struct syterkit_efex_result syterkit_efex_result __attribute__((section(".efex.result"), used));

void syterkit_efex_set_dram_result(const uint32_t *parameters, size_t count)
{
	syterkit_efex_set_dram_result_status(parameters, count, 1U);
}

void syterkit_efex_set_dram_result_status(const uint32_t *parameters, size_t count, uint32_t initialized)
{
	size_t index;

	for (index = 0; index < SYTERKIT_EFEX_DRAM_PARA_WORDS; index++)
		syterkit_efex_result.dram_paras[index] = (parameters != NULL && index < count) ? parameters[index] : 0;

	syterkit_efex_result.dram_update_flag = initialized ? 1U : 0U;
	syterkit_efex_result.dram_init_flag = SYTERKIT_EFEX_DRAM_MAGIC;
}

void syterkit_efex_arch_sync(void)
{
	flush_dcache_all();
#if defined(CONFIG_ARCH_ARM32)
	arm32_icache_invalidate_all();
	__asm__ volatile("dsb sy" ::: "memory");
	__asm__ volatile("isb" ::: "memory");
#else
	__asm__ volatile("fence rw, rw" ::: "memory");
	__asm__ volatile("fence.i" ::: "memory");
	__asm__ volatile(".long 0x01b0000b" ::: "memory");
#endif
}
