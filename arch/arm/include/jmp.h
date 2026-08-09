/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _JMP_H_
#define _JMP_H_

/**
 * @brief Enable the ARM SMP coherency bit before entering a kernel.
 */
static inline void enable_kernel_smp(void) {
	// Read ACTLR from coprocessor 15 (CP15), register c1
	asm volatile("MRC p15, 0, r0, c1, c0, 1");
	// Perform bitwise OR operation on register r0 with 0x040,
	// setting bit 6 This is to set the 6th bit of the ACTLR register
	asm volatile("ORR r0, r0, #0x040");
	// Write the value in register r0 to coprocessor 15 (CP15),
	// register c1 Writing back the modified value to the ACTLR register
	asm volatile("MCR p15, 0, r0, c1, c0, 1");
}

/**
 * @brief Transfer control to a raw ARM entry address.
 *
 * @param[in] addr Entry address. The function does not return.
 */
static inline void syterkit_jmp(uint32_t addr) {
	// Move the constant value 0 into register r2
	asm volatile("mov r2, #0");

	// Execute privileged instruction using coprocessor 15 (CP15),
	// writing the value of register r2 to the Data ram latency
	// control bit of the Cache Size Selection Register (CSSR)
	// This sets the Data ram latency control bit of CSSR to 0
	asm volatile("mcr p15, 0, r2, c7, c5, 6");

	// Branch to the address stored in register r0,
	// performing an unconditional jump to that location
	asm volatile("bx r0");
}

/** @brief Return control to the SoC FEL entry point. */
static inline void jmp_to_fel(void) {
	syterkit_jmp(0x20);
}

/**
 * @brief Enter a 32-bit kernel with its device tree address.
 *
 * @param[in] addr Kernel entry address.
 * @param[in] fdt Flattened device tree address passed in register r2.
 */
static inline void syterkit_jmp_kernel(uint32_t addr, uint32_t fdt) {
	void (*kernel_entry)(int zero, int arch, unsigned int params);
	kernel_entry = (void (*)(int, int, unsigned int)) addr;
	kernel_entry(0, ~0, (unsigned int) fdt);
}


#endif// _JMP_H_
