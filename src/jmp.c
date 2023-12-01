/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <common.h>
#include <arm32.h>

#include <log.h>

void enable_kernel_smp(void)
{
	asm volatile("MRC p15, 0, r0, c1, c0, 1");  // Read ACTLR
	asm volatile("ORR r0, r0, #0x040");         // Set bit 6
	asm volatile("MCR p15, 0, r0, c1, c0, 1");  // Write ACTLR
}

void syterkit_jmp(uint32_t addr)
{
    asm volatile("mov r2, #0");
    asm volatile("mcr p15, 0, r2, c7, c5, 6");
    asm volatile("bx r0");
}

void syterkit_jmp_kernel(uint32_t addr, uint32_t fdt)
{
    void (*kernel_entry)(int zero, int arch, unsigned int params);
    kernel_entry = (void (*)(int, int, unsigned int))addr;
    kernel_entry(0, ~0, (unsigned int)fdt);
}
