/* SPDX-License-Identifier: GPL-2.0+ */

/* System map of the E907: what the data cache may hold. Called before the caches are switched on. */

#include <mmu.h>
#include <e907/sysmap.h>

#define SRAM_BASE 0x00100000U
#define SRAM_SIZE 0x00020000U
#define DRAM_BASE 0x40000000U
#define DRAM_SIZE 0x08000000U /* 128 MB */

void board_sysmap_init(void)
{
	/* the SRAM and the DRAM are cacheable; everything between them and the registers inside it are device regions */
	sysmap_add_mem_region(SRAM_BASE, SRAM_SIZE, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(DRAM_BASE, DRAM_SIZE, SYSMAP_MEM_ATTR_RAM);
}
