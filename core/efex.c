/* SPDX-License-Identifier: GPL-2.0+ */

#include <efex.h>

#include <cache.h>

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
