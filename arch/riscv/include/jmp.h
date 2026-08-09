/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RISCV_JMP_H__
#define __RISCV_JMP_H__

#include <stdint.h>
#include <types.h>

/**
 * @brief Transfer control to a raw RISC-V entry address.
 *
 * @param[in] addr Entry address. The function does not return.
 */
static inline __attribute__((noreturn)) void syterkit_jmp(uintptr_t addr) {
	void (*entry)(void) = (void (*)(void)) addr;

	asm volatile("fence.i" ::: "memory");
	entry();
	__builtin_unreachable();
}

/** @brief Return control to the SoC FEL entry point. */
static inline __attribute__((noreturn)) void jmp_to_fel(void) {
	syterkit_jmp(0x20);
}

#endif /* __RISCV_JMP_H__ */
