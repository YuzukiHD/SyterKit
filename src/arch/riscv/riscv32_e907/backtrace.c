/* SPDX-License-Identifier: GPL-2.0+ */

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_DEBUG
#define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG
#endif

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#define CONFIG_ARCH_RISCV32 1

/**
 * @brief Maximum limit for backtrace scanning.
 * 
 * This constant defines the maximum limit for scanning the backtrace. 
 * The backtrace function will scan instructions and stack frames 
 * up to this size. If the limit is exceeded, the backtrace operation will fail.
 */
#define BT_SCAN_MAX_LIMIT 0xFFFFFF

/**
 * @brief Maximum number of backtrace levels.
 * 
 * This constant defines the maximum depth of the backtrace. It limits 
 * how many stack frames the backtrace function can unwind before stopping.
 */
#define BT_LEVEL_LIMIT 64

/**
 * @brief Converts a program counter (PC) value to an address.
 * 
 * This macro takes a program counter (PC) value and converts it to 
 * a valid address by clearing the least significant bit. This operation
 * is commonly used to align PC addresses properly for instruction fetch.
 * 
 * @param pc The program counter value to convert.
 * @return The corresponding address, aligned to a 2-byte boundary.
 */
#define PC2ADDR(pc) ((char *) (((uint32_t) (pc)) & 0xfffffffe))

/**
 * @brief Determines the length of an instruction in bytes based on its encoding.
 * 
 * This macro computes the length of an instruction in bytes, given a 32-bit
 * instruction encoding. The length is determined based on specific bit patterns
 * in the instruction word.
 * 
 * @param x The instruction encoding (32-bit).
 * @return The length of the instruction in bytes (either 2, 4, 6, or 8).
 */
#define insn_length(x) (((x) &0x03) < 0x03 ? 2 : ((x) &0x1f) < 0x1f ? 4 \
										 : ((x) &0x3f) < 0x3f		? 6 \
																	: 8)

/**
 * @brief Extracts a specific bit field from a value.
 * 
 * This macro extracts a bit field from a value `x` based on the specified 
 * high and low bit positions. The bits are masked and shifted to the 
 * right, leaving the desired bit field.
 * 
 * @param x The value from which to extract the bit field.
 * @param high The index of the highest bit (inclusive).
 * @param low The index of the lowest bit (inclusive).
 * @return The extracted bit field.
 */
#define BITS(x, high, low) ((x) & (((1 << ((high) - (low) + 1)) - 1) << (low)))

/**
 * @brief Pointer to the start of the image in memory.
 * 
 * This variable marks the start address of the memory region that
 * contains the program image. It is used to check if a program counter
 * (PC) value falls within the valid range of executable memory.
 */
extern uint8_t __spl_start[];

/**
 * @brief Pointer to the end of the stack service region.
 * 
 * This variable marks the end address of the memory region used for the
 * stack service. It is used to check if a program counter (PC) value falls
 * within the valid memory region, preventing access to out-of-bounds memory.
 */
extern uint8_t __stack_srv_end[];

/**
 * @brief Checks whether a given program counter (PC) address is within a valid range.
 * 
 * This function checks if the provided PC (program counter) value lies within
 * the memory region between `__spl_start` and `__stack_srv_end`. It ensures that
 * the PC value points to a valid memory address in the executable region of the program.
 * 
 * @param pc The program counter value to check.
 * @return 1 if the PC is within the valid address range, 0 otherwise.
 */
static int inline backtrace_check_address(void *pc) {
	if (((uint32_t) pc > (uint32_t) __spl_start) && ((uint32_t) pc < (uint32_t) __stack_srv_end)) {
		return 1; /**< Valid address in the range. */
	}
	return 0; /**< Invalid address outside the range. */
}

/**
 * @brief Finds the offset of the Link Register (LR) from the return address in a backtrace.
 * 
 * This function attempts to determine the offset of the Link Register (LR) from the return 
 * address in a backtrace. It checks whether the LR corresponds to an IRQ handler exit address 
 * and adjusts accordingly. The function also checks the validity of the address and computes 
 * the instruction length based on the encoded instruction in memory.
 * 
 * @param LR The address of the Link Register (LR) to be checked.
 * @return The offset from the LR to the return address if valid, 0 if invalid or the address is 
 *         within an IRQ handler exit region.
 */
static int riscv_backtrace_find_lr_offset(char *LR) {
	char *LR_fixed;
	uint16_t ins16;
	int offset = 4;				/**< Initial offset value for standard instruction length. */
	uint64_t *irq_entry = NULL; /**< Pointer to the IRQ entry (interrupt handler entry address). */

	LR_fixed = LR;

	/* Check if the LR corresponds to the IRQ handler exit address. */
	if (LR_fixed == PC2ADDR(irq_entry)) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n", irq_entry); /**< Log IRQ entry address if match found. */
		return 0;													   /**< Return 0, indicating no valid offset. */
	}

	/* Validate that the address (LR - 4) points to executable code. */
	if (backtrace_check_address(LR_fixed - 4) == 0) {
		return 0; /**< Return 0 if the address is invalid (not in valid text region). */
	}

	/* Retrieve the instruction at LR - 4 and compute the instruction length. */
	ins16 = *(uint16_t *) (LR_fixed - 4); /**< Fetch the instruction at LR-4. */
	offset = insn_length(ins16);		  /**< Compute the length of the instruction. */

	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n", LR_fixed - offset); /**< Log the address after offset adjustment. */

	return offset; /**< Return the computed offset. */
}

/**
 * @brief Determines the frame size for a push to the link register (LR) in a RISC-V instruction.
 * 
 * This function analyzes a 32-bit RISC-V instruction to determine whether it corresponds to a
 * "push" operation to the link register (LR) or the stack pointer (SP), and computes the 
 * associated frame size or offset. The function specifically handles instructions that 
 * manipulate the stack pointer or the link register.
 * 
 * @param inst The 32-bit instruction to analyze.
 * @param offset A pointer to an integer where the computed offset (in terms of long size) will be stored.
 * @return int Returns 0 if the instruction is valid and corresponds to a push operation, -1 otherwise.
 */
int riscv_ins32_get_push_lr_framesize(uint32_t inst, int *offset) {
	int ret = -1;

	printk_trace("BT: inst:0x%x\n", inst);

	// Check for 'sd ra, (offset)sp' instruction
	if ((inst & 0x01FFF07F) == 0x113023) {
		/* sd ra, (offset)sp  */
		int immed = (inst & 0xF80);
		immed >>= 7;
		immed |= ((inst & 0xFE000000) >> 25) << 5;
		if (((immed >> 11) & 0x01) != 0) {
			immed = 0xFFF - immed + 1;
		}
		*offset = immed / sizeof(long);
		ret = -1;
	}
	// Check for 'addi sp, sp, #imm' instruction
	else if ((inst & 0x000FFFFF) == 0x10113) {
		/*  addi sp, sp, #imm  */
		int immed = BITS(inst, 31, 20);
		immed >>= 20;
		immed &= 0xFFF;
		if ((immed >> 11) != 0) {
			immed = 0xFFF - immed + 1;
			ret = 0;
		} else {
			ret = -1;
		}
	}
#if !defined(CONFIG_ARCH_RISCV32) /* RISCV32 not support addiw sp, sp, #imm */
	// Check for 'addiw sp, sp, #imm' instruction
	else if ((inst & 0x000FFFFF) == 0x1011B) {
		/*  addiw sp, sp, #imm  */
		int immed = BITS(inst, 31, 20);
		immed >>= 20;
		immed &= 0xFFF;
		if ((immed >> 11) != 0) {
			immed = 0xFFF - immed + 1;
			ret = 0;
		} else {
			ret = -1;
		}
	}
#endif

	return ret;
}

/**
 * @brief Determines the frame size for a push to the link register (LR) in a RISC-V compressed instruction.
 * 
 * This function analyzes a 16-bit RISC-V compressed instruction (C-extension) to determine if it corresponds
 * to a "push" operation to the link register (LR) or the stack pointer (SP), and computes the associated 
 * frame size or offset. The function specifically handles instructions that manipulate the stack pointer
 * or the link register in compressed RISC-V instructions.
 *
 * @param inst The 16-bit compressed instruction to analyze.
 * @param offset A pointer to an integer where the computed offset (in terms of long size) will be stored.
 * 
 * @return int Returns 0 if the instruction is valid and corresponds to a push operation, -1 otherwise.
 */
static int riscv_ins16_get_push_lr_framesize(uint16_t inst, int *offset) {
	int ret = -1;

	printk_trace("BT: inst:0x%x: \n", inst);

	// Check for 'c.sdsp ra, (offset)sp' instruction
	if ((inst & 0xE07E) == 0xE006) {
		/* c.sdsp ra, (offset)sp  */
		int immed_6_8 = (inst >> 7) & 0x07;
		int immed_3_5 = (inst >> 10) & 0x07;
		int immed = immed_6_8 << 6 | immed_3_5 << 3;
		*offset = immed / sizeof(long);
		printk_trace("BT: \tc.sdsp ra, (offset%p)sp, #immed=%d \n", offset, immed);
		ret = -1;
	}
	// Check for 'c.swsp ra, (offset)sp' instruction
	else if ((inst & 0xE07E) == 0xC006) {
		/* c.swsp ra, (offset)sp  */
		int immed_6_7 = (inst >> 7) & 0x03;
		int immed_2_5 = (inst >> 9) & 0x0f;
		int immed = immed_6_7 << 6 | immed_2_5 << 2;
		*offset = immed / sizeof(long);
		printk_trace("BT: \tc.swsp ra, (offset%p)sp, #immed=%d \n", offset, immed);
		ret = -1;
	}
	// Check for 'c.addi16sp #imm' instruction
	else if ((inst & 0xEF83) == 0x6101) {
		/*  c.addi16sp #imm  */
		int immed_5 = (inst >> 2) & 0x01;
		int immed_7_8 = (inst >> 3) & 0x3;
		int immed_6 = (inst >> 5) & 0x1;
		int immed_4 = (inst >> 6) & 0x1;
		int immed_9 = (inst >> 12) & 0x1;
		int immed = immed_5 << 5 | immed_7_8 << 7 | immed_6 << 6 | immed_4 << 4 | immed_9 << 9;

		printk_trace("BT: \tc.addi16sp #immed=%d \n", immed);

		if ((immed >> 9) != 0) {
			immed = 0x3FF - immed + 1;
			ret = 0;
		} else {
			ret = -1;
		}
	}
	// Check for 'c.addi sp, sp, #imm' instruction
	else if ((inst & 0xEF03) == 0x101) {
		/*  c.addi sp, sp, #imm  */
		int immed_0_4 = (inst >> 2) & 0x1F;
		int immed_5 = (inst >> 12) & 0x1;
		int immed = immed_5 << 5 | immed_0_4;

		printk_trace("BT: \tc.addi sp, sp, #immed=%d \n", immed);

		if ((immed >> 5) != 0) {
			immed = 0x3F - immed + 1;
			ret = 0;
		} else {
			ret = -1;
		}
	}
#if !defined(CONFIG_ARCH_RISCV32) /* RISCV32 not support c.addiw sp */
	// Check for 'c.addiw sp, #imm' instruction
	else if ((inst & 0xEF03) == 0x2101) {
		/*  c.addiw sp, #imm  */
		int immed_0_4 = (inst >> 2) & 0x1F;
		int immed_5 = (inst >> 12) & 0x1;
		int immed = immed_5 << 5 | immed_0_4;

		printk_trace("BT: \tc.addiw sp, #immed=%d \n", immed);

		if ((immed >> 5) != 0) {
			immed = 0x3F - immed + 1;
			ret = 0;
		} else {
			ret = -1;
		}
	}
#endif

	printk_trace("BT: \tret = %d\n", ret);
	return ret;
}

/**
 * @brief Checks if a RISC-V instruction corresponds to stack push instructions 
 *        like `addi sp, sp, imm` and `addiw sp, sp, imm`, and calculates the 
 *        effective stack push size based on the immediate value.
 *
 * This function examines a 32-bit RISC-V instruction and checks if it corresponds 
 * to one of two stack adjustment operations: 
 * - `addi sp, sp, imm` (add immediate to the stack pointer)
 * - `addiw sp, sp, imm` (add immediate to the stack pointer, with word extension).
 *
 * If the instruction matches one of these operations, it calculates the immediate 
 * value and adjusts the stack pointer accordingly. The size of the stack push 
 * is calculated based on the immediate value, and the function returns this 
 * value divided by the size of a `long` type.
 *
 * If the instruction does not correspond to one of the stack push operations, 
 * the function returns `-1`.
 *
 * @param inst The 32-bit RISC-V instruction to check.
 * 
 * @return int Returns the stack push size in terms of the number of `long` units 
 *             if the instruction corresponds to a valid stack push operation, 
 *             otherwise returns `-1`.
 *
 * @note The instruction must modify the stack pointer (`sp`) to be considered valid. 
 *       The immediate value is expected to be a signed 12-bit value and may represent 
 *       a positive or negative offset to the stack pointer.
 */
int riscv_ins32_backtrace_stask_push(uint32_t inst) {
	int ret = -1;

	if ((inst & 0x000FFFFF) == 0x10113) {
		/*  addi sp, sp, #imm  */
		int immed = BITS(inst, 31, 20);
		immed >>= 20;
		immed &= 0xFFF;
		if ((immed >> 11) != 0) {
			immed = 0xFFF - immed + 1;
			ret = immed / sizeof(long);
		} else {
			ret = -1;
		}
	}
#if !defined(CONFIG_ARCH_RISCV32) /* RISCV32 not support addiw sp, sp, #imm */
	else if ((inst & 0x000FFFFF) == 0x1011B) {
		/*  addiw sp, sp, #imm  */
		int immed = BITS(inst, 31, 20);
		immed >>= 20;
		immed &= 0xFFF;
		if ((immed >> 11) != 0) {
			immed = 0xFFF - immed + 1;
			ret = immed / sizeof(long);
		} else {
			ret = -1;
		}
	}
#endif
	printk_trace("BT: inst:0x%x, ret = %d\n", inst, ret);

	return ret;
}

/**
 * @brief Processes compressed RISC-V instructions related to stack pointer adjustments.
 * 
 * This function checks if a 16-bit compressed RISC-V instruction corresponds to one of the 
 * following stack pointer modification operations:
 * - `c.addi16sp` (compressed `addi16sp` instruction, used for modifying the stack pointer with a 9-bit signed immediate)
 * - `c.addi sp` (compressed `addi` instruction for modifying `sp` with a 6-bit immediate)
 * - `c.addiw sp` (compressed `addiw` instruction for modifying `sp` with a 6-bit immediate)
 *
 * If the instruction matches one of these formats, it extracts the immediate value and calculates the 
 * corresponding adjustment to the stack pointer, returning the size of the adjustment in terms of `long` units. 
 * The function returns `-1` if the instruction doesn't correspond to any recognized format or if the immediate 
 * value does not result in a valid stack adjustment.
 *
 * @param inst The 16-bit compressed RISC-V instruction to be analyzed.
 * 
 * @return int Returns the stack adjustment size (in terms of `long` units) if the instruction is valid, 
 *             otherwise returns `-1`.
 *
 * @note 
 * - The function assumes that the instruction is valid and checks for stack pointer adjustments only.
 * - For `c.addi16sp`, the immediate is a 9-bit signed value that modifies the stack pointer (`sp`).
 * - For `c.addi sp, sp, imm` and `c.addiw sp, imm`, the immediate is a 6-bit signed value used to modify `sp`.
 * - The function divides the immediate value by `sizeof(long)` to return the number of `long` units adjusted.
 */
static int riscv_ins16_backtrace_stask_push(uint32_t inst) {
	int ret = -1;

	if ((inst & 0xEF83) == 0x6101) {
		/*  c.addi16sp #imm  */
		int immed_4 = (inst >> 6) & 0x01;
		int immed_5 = (inst >> 2) & 0x01;
		int immed_6 = (inst >> 5) & 0x01;
		int immed_7_8 = (inst >> 3) & 0x3;
		int immed_9 = (inst >> 12) & 0x1;
		int immed = (immed_4 << 4) | (immed_5 << 5) | (immed_6 << 6) | (immed_7_8 << 7) | (immed_9 << 9);
		if ((immed >> 9) != 0) {
			immed = 0x3FF - immed + 1;
			ret = immed / sizeof(long);
		} else {
			ret = -1;
		}
	} else if ((inst & 0xEF03) == 0x101) {
		/*  c.addi sp, sp, #imm  */
		int immed_5 = (inst >> 12) & 0x01;
		int immed_0_4 = (inst >> 2) & 0x1F;
		int immed = (immed_0_4) | (immed_5 << 5);
		if ((immed >> 5) != 0) {
			immed = 0x3F - immed + 1;
			ret = immed / sizeof(long);
		} else {
			ret = -1;
		}
	}
#if !defined(CONFIG_ARCH_RISCV32) /* RISCV32 not support c.addiw sp */
	else if ((inst & 0xEF03) == 0x2101) {
		/*  c.addiw sp, #imm  */
		int immed_5 = (inst >> 12) & 0x01;
		int immed_0_4 = (inst >> 2) & 0x1F;
		int immed = (immed_0_4) | (immed_5 << 5);
		if ((immed >> 5) != 0) {
			immed = 0x3F - immed + 1;
			ret = immed / sizeof(long);
		} else {
			ret = -1;
		}
	}
#endif

	printk_trace("BT: inst:0x%x, ret = %d\n", inst, ret);

	return ret;
}

/**
 * @brief Perform a backtrace from the current stack pointer and program counter.
 *
 * This function walks the stack to retrieve the calling function's information 
 * by analyzing the return address (`LR`) and corresponding program counter (`PC`) values. 
 * It starts by looking for instructions that push the link register (`LR`) to the stack, 
 * then works backwards through the stack frames. The function handles both 16-bit and 
 * 32-bit instructions in RISC-V architecture.
 *
 * The function attempts to find the appropriate return address and adjusts the 
 * stack pointer (`SP`) accordingly. It then calculates the program counter (`PC`) 
 * from the link register (`LR`) by finding the corresponding offset. The function 
 * is responsible for determining whether the address information is valid and updates 
 * the `SP` and `PC` pointers.
 *
 * The function returns `1` if a valid backtrace is successfully performed, 
 * or `0` if the link register's offset is zero. If an error is encountered 
 * at any point, the function prints an error message and returns `-1`.
 *
 * @param pSP Pointer to the stack pointer (`SP`) to be updated.
 * @param pPC Pointer to the program counter (`PC`) to be updated.
 *
 * @return int Returns:
 *         - `1` if a valid backtrace is successfully performed.
 *         - `0` if the link register's offset is zero.
 *         - `-1` in case of failure (e.g., invalid addresses or instructions).
 *
 * @note The backtrace relies on the presence of valid instruction addresses and 
 *       will fail if the stack pointer or program counter point to invalid regions. 
 *       The function also uses `insn_length()` to determine instruction lengths 
 *       and adjusts the stack and program counter accordingly.
 */
static int riscv_backtrace_from_stack(long **pSP, char **pPC, char **pLR) {
	char *parse_addr = NULL;
	long *SP = *pSP;
	char *PC = *pPC;
	char *LR = *pLR;
	int i, temp, framesize = 0, offset = 0, result = 0;
	uint32_t ins32 = 0;
	uint16_t ins16 = 0, ins16_h = 0, ins16_l = 0;

	for (i = 2; i < BT_SCAN_MAX_LIMIT; i += 2) {
		int result = 0;
		parse_addr = PC - i;
		if (backtrace_check_address(parse_addr) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr);
			return -1;
		}
		ins16_h = *(uint16_t *) parse_addr;

		if (backtrace_check_address(parse_addr - 2) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr - 2);
			return -1;
		}
		ins16_l = *(uint16_t *) (parse_addr - 2);

		if (insn_length(ins16_l) == 4) {
			printk_trace("BT: insn len == 4, parse_addr = %p:\n", parse_addr);
			ins32 = (ins16_h << 16) | ins16_l;
			result = riscv_ins32_get_push_lr_framesize(ins32, &offset);
			i += 2;
		} else {
			printk_trace("BT: insn len == 2, parse_addr = %p:\n", parse_addr);
			ins16 = ins16_h;
			result = riscv_ins16_get_push_lr_framesize(ins16, &offset);
		}

		if (result >= 0) {
			break;
		}
	}

	parse_addr = PC - i;

	printk_trace("BT: i = %d, parse_addr = %p, PC = %p, offset = %d\n", i, parse_addr, PC, offset);

	if (i == BT_SCAN_MAX_LIMIT) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. scope overflow\n");
		return -1;
	}

	for (i = 0; parse_addr + i < PC; i += 2) {
		if (backtrace_check_address(parse_addr + i) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr + i);
			return -1;
		}
		ins16_l = *(uint16_t *) (parse_addr + i);

		if (backtrace_check_address(parse_addr + i + 2) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr + i + 2);
			return -1;
		}
		ins16_h = *(uint16_t *) (parse_addr + i + 2);

		if (insn_length(ins16_l) == 4 || ins16_l == 0) {
			ins32 = (ins16_h << 16) | ins16_l;
			temp = riscv_ins32_backtrace_stask_push(ins32);
			i += 2;
		} else {
			ins16 = ins16_l;
			temp = riscv_ins16_backtrace_stask_push(ins16);
		}
		if (temp >= 0) {
			framesize += temp;
		}
	}

	printk_trace("BT: i = %d, framesize = %d, SP = %p\n", i, framesize, SP);

	if (!offset) {
		return -1;
	}

	if (backtrace_check_address(SP + offset) == 0)
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", SP + offset);

	LR = (char *) *(SP + offset);
	if (backtrace_check_address(LR) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR);
		return -1;
	}
	*pSP = SP + framesize;
	offset = riscv_backtrace_find_lr_offset(LR);
	*pPC = LR - offset;

	printk_trace("BT: *pSP = %p, offset = %d, *pPC = %p\n", *pSP, offset, *pPC);

	return offset == 0 ? 1 : 0;
}

/**
 * @brief Performs a backtrace from the current stack frame.
 * 
 * This function is used to trace the call stack, starting from the provided program counter
 * and stack pointer. Before performing the backtrace, it checks if the program counter is
 * valid (i.e., points to executable code). If the program counter is invalid, the function
 * returns an error code (-1). Otherwise, it delegates the actual backtrace work to
 * `riscv_backtrace_from_stack`.
 *
 * @param[in,out] pSP Pointer to the stack pointer. The stack pointer is updated with the
 *                    result of the backtrace.
 * @param[in,out] pPC Pointer to the program counter. The program counter is updated with
 *                    the result of the backtrace.
 * 
 * @return 0 if the backtrace is successful and the program counter is updated correctly,
 *         or -1 if an invalid program counter is provided or the backtrace fails.
 */
static int backtrace_from_stack(long **pSP, char **pPC, char **pLR) {
	if (backtrace_check_address(*pPC) == 0) {
		return -1;
	}

	return riscv_backtrace_from_stack(pSP, pPC, pLR);
}

/**
 * @brief Handles the backtrace return pop for a 32-bit instruction.
 * 
 * This function checks if the provided 32-bit instruction matches the return 
 * address instruction and returns the corresponding result.
 * 
 * @param inst The 32-bit instruction to check.
 * @return 0 if the instruction matches the return instruction, -1 otherwise.
 */
static int riscv_ins32_backtrace_return_pop(uint32_t inst) {
	int ret = -1; /**< Default return value is -1, indicating no match. */

	if ((inst) == 0x00008067) { /**< Check if instruction matches return instruction. */
		ret = 0;				/**< Set return value to 0 if it matches. */
	}

	printk_trace("BT: inst:0x%x, ret = %d\n", inst, ret); /**< Log the instruction and return value. */
	return ret;											  /**< Return the result. */
}

/**
 * @brief Handles the backtrace return pop for a 16-bit instruction.
 * 
 * This function checks if the provided 16-bit instruction matches the return 
 * address instruction and returns the corresponding result.
 * 
 * @param inst The 16-bit instruction to check.
 * @return 0 if the instruction matches the return instruction, -1 otherwise.
 */
static int riscv_ins16_backtrace_return_pop(uint16_t inst) {
	int ret = -1; /**< Default return value is -1, indicating no match. */

	if ((inst) == 0x8082) { /**< Check if instruction matches return instruction. */
		ret = 0;			/**< Set return value to 0 if it matches. */
	}

	printk_trace("BT: inst:0x%x, ret = %d\n", inst, ret); /**< Log the instruction and return value. */
	return ret;											  /**< Return the result. */
}

/**
 * @brief Handles the backtrace stack pop for a 32-bit instruction.
 * 
 * This function checks if the instruction is a stack pointer adjustment (addi or addiw) 
 * and computes the corresponding stack adjustment. It handles both immediate values 
 * and calculates the stack width.
 * 
 * @param inst The 32-bit instruction to check.
 * @return The computed stack pop value, or -1 if no match is found.
 */
static int riscv_ins32_backtrace_stack_pop(unsigned int inst) {
	int ret = -1;					/**< Default return value is -1, indicating no match. */
	int stack_width = sizeof(long); /**< Define stack width based on long data type size. */

	/*  Check for "addi sp, sp, #imm" instruction. */
	if ((inst & 0x000FFFFF) == 0x10113) {
		int immed = BITS(inst, 31, 20); /**< Extract immediate value from instruction. */
		immed >>= 20;					/**< Shift to get the immediate. */
		immed &= 0xFFF;					/**< Mask to get 12-bit immediate. */
		if ((immed >> 11) != 0) {
			ret = -1; /**< Invalid immediate, return -1. */
		} else {
			immed = 0xFFF - immed + 1; /**< Calculate adjusted immediate. */
			ret = immed / stack_width; /**< Convert to stack adjustment. */
		}
		printk_trace("BT: \t addi sp, sp, #immed=%d \n", immed); /**< Log instruction and immediate value. */
	}
#if !defined(CONFIG_ARCH_RISCV32) /* RISCV32 does not support "addiw sp, sp, #imm" instruction */
	else if ((inst & 0x000FFFFF) == 0x1011B) {
		/*  Check for "addiw sp, sp, #imm" instruction. */
		int immed = BITS(inst, 31, 20); /**< Extract immediate value from instruction. */
		immed >>= 20;					/**< Shift to get the immediate. */
		immed &= 0xFFF;					/**< Mask to get 12-bit immediate. */
		if ((immed >> 11) != 0) {
			ret = -1; /**< Invalid immediate, return -1. */
		} else {
			immed = 0xFFF - immed + 1; /**< Calculate adjusted immediate. */
			ret = immed / stack_width; /**< Convert to stack adjustment. */
		}
		printk_trace("BT: \t addiw sp, sp, #immed=%d \n", immed); /**< Log instruction and immediate value. */
	}
#endif

	printk_trace("BT: inst:0x%x, ret:%d\n", inst, ret); /**< Log instruction and return value. */
	return ret;											/**< Return the result. */
}

/**
 * @brief Handles the backtrace stack pop for a 16-bit instruction.
 * 
 * This function checks if the provided 16-bit instruction is a stack pointer 
 * adjustment (either `c.addi16sp`, `c.addi sp`, or `c.addiw sp`) and computes 
 * the corresponding stack adjustment.
 * 
 * @param inst The 16-bit instruction to check.
 * @return The computed stack pop value, or -1 if no valid stack pop is found.
 */
static int riscv_ins16_backtrace_stack_pop(uint16_t inst) {
	int ret = -1;					/**< Default return value is -1, indicating no match. */
	int stack_width = sizeof(long); /**< Define stack width based on long data type size. */

	/*  Check for "c.addi16sp #imm" instruction. */
	if ((inst & 0xEF83) == 0x6101) {
		int immed_4 = (inst >> 6) & 0x01;																  /**< Extract bit 6 for immediate part. */
		int immed_5 = (inst >> 2) & 0x01;																  /**< Extract bit 2 for immediate part. */
		int immed_6 = (inst >> 5) & 0x01;																  /**< Extract bit 5 for immediate part. */
		int immed_7_8 = (inst >> 3) & 0x3;																  /**< Extract bits 3-4 for immediate part. */
		int immed_9 = (inst >> 12) & 0x1;																  /**< Extract bit 12 for immediate part. */
		int immed = (immed_4 << 4) | (immed_5 << 5) | (immed_6 << 6) | (immed_7_8 << 7) | (immed_9 << 9); /**< Combine extracted bits into a full immediate value. */

		if ((immed >> 9) != 0) {	   /**< If the immediate value is too large, adjust it. */
			immed = 0x3FF - immed + 1; /**< Adjust the immediate for negative values. */
		}

		printk_trace("BT: \tc.addi16sp #immed=%d \n", immed); /**< Log the immediate value. */
		ret = immed / stack_width;							  /**< Calculate the stack pop based on the immediate value. */
	}
	/*  Check for "c.addi sp, sp, #imm" instruction. */
	else if ((inst & 0xEF03) == 0x101) {
		int immed_5 = (inst >> 12) & 0x01;		  /**< Extract bit 12 for immediate part. */
		int immed_0_4 = (inst >> 2) & 0x1F;		  /**< Extract bits 2-6 for immediate part. */
		int immed = (immed_0_4) | (immed_5 << 5); /**< Combine bits to form immediate value. */

		if ((immed >> 5) != 0) { /**< Check if immediate value is too large. */
			ret = -1;			 /**< Return error if the immediate is invalid. */
		} else {
			immed = 0x3F - immed + 1; /**< Adjust the immediate value for negative values. */
		}

		printk_trace("BT: \tc.addi sp, sp, #immed=%d \n", immed); /**< Log the immediate value. */
		ret = immed / stack_width;								  /**< Calculate the stack pop based on the immediate value. */
	}
#if !defined(CONFIG_ARCH_RISCV32) /* RISCV32 does not support "c.addiw sp, #imm" instruction */
	/*  Check for "c.addiw sp, #imm" instruction. */
	else if ((inst & 0xEF03) == 0x2101) {
		int immed_5 = (inst >> 12) & 0x01;		  /**< Extract bit 12 for immediate part. */
		int immed_0_4 = (inst >> 2) & 0x1F;		  /**< Extract bits 2-6 for immediate part. */
		int immed = (immed_0_4) | (immed_5 << 5); /**< Combine bits to form immediate value. */

		if ((immed >> 5) != 0) { /**< Check if immediate value is too large. */
			ret = -1;			 /**< Return error if the immediate is invalid. */
		} else {
			immed = 0x3F - immed + 1; /**< Adjust the immediate value for negative values. */
		}

		printk_trace("BT: \tc.addiw sp, #immed=%d \n", immed); /**< Log the immediate value. */
		ret = immed / stack_width;							   /**< Calculate the stack pop based on the immediate value. */
	}
#endif

	printk_trace("BT: inst:0x%x\n", inst); /**< Log the instruction. */
	return ret;							   /**< Return the result. */
}

/**
 * @brief Reconstructs a backtrace using the link register (LR) and program counter (PC).
 * 
 * This function inspects the instructions at the current PC and LR, computes the stack 
 * frames, and updates the SP and PC accordingly. It tries to decode 16-bit and 32-bit 
 * instructions and handles different instruction formats for backtrace purposes.
 * 
 * @param pSP Pointer to the stack pointer to be updated.
 * @param pPC Pointer to the program counter to be updated.
 * @param LR Link register (return address) to start the backtrace from.
 * 
 * @return 1 if successful, 0 if the link register offset is zero, -1 on failure.
 */
static int riscv_backtrace_from_lr(long **pSP, char **pPC, char *LR) {
	long *SP = *pSP;									/**< Local stack pointer. */
	char *PC = *pPC;									/**< Local program counter. */
	char *parse_addr = NULL;							/**< Temporary address for instruction parsing. */
	int i, temp, framesize = 0, offset = 0, result = 0; /**< Loop counters, temporary values, and result variables. */
	uint32_t ins32 = 0;									/**< 32-bit instruction. */
	uint16_t ins16 = 0, ins16_h = 0, ins16_l = 0;		/**< 16-bit instruction (low and high). */

	/* Check if the current program counter is valid. */
	if (backtrace_check_address(PC) == 0) {
		/* Check if the link register (LR) is valid. */
		if (backtrace_check_address(LR) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR); /**< Log invalid LR. */
			return -1;
		}
		offset = riscv_backtrace_find_lr_offset(LR); /**< Find the LR offset. */
		PC = LR - offset;							 /**< Update PC based on LR offset. */
		*pPC = PC;									 /**< Set the new program counter. */
		return offset == 0 ? 1 : 0;					 /**< Return success if offset is 0, otherwise return 0. */
	}

	/* Scan the instructions at the current PC for backtrace. */
	for (i = 0; i < BT_SCAN_MAX_LIMIT; i += 2) {
		parse_addr = PC + i; /**< Calculate the address for parsing instructions. */

		/* Check if the current address is valid. */
		if (backtrace_check_address(parse_addr) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr); /**< Log failure. */
			return -1;
		}

		/* Check the next address for a valid instruction. */
		if (backtrace_check_address(parse_addr + 2) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr + 2); /**< Log failure. */
			return -1;
		}

		ins16_l = *(uint16_t *) parse_addr;		  /**< Fetch the low 16-bit instruction. */
		ins16_h = *(uint16_t *) (parse_addr + 2); /**< Fetch the high 16-bit instruction. */

		/* Check if the instruction length is 4 bytes or invalid, then combine into 32-bit. */
		if (insn_length(ins16_l) == 4 || ins16_l == 0) {
			ins32 = (ins16_h << 16) | ins16_l;				  /**< Combine high and low 16-bits into a 32-bit instruction. */
			result = riscv_ins32_backtrace_return_pop(ins32); /**< Check if it's a return pop for 32-bit instruction. */
			i += 2;											  /**< Adjust the loop index for 32-bit instruction. */
			parse_addr -= 4;								  /**< Move back by 4 bytes for 32-bit instruction. */
		} else {
			ins16 = ins16_l;								  /**< Use the 16-bit instruction if it's valid. */
			result = riscv_ins16_backtrace_return_pop(ins16); /**< Check if it's a return pop for 16-bit instruction. */
			parse_addr -= 2;								  /**< Move back by 2 bytes for 16-bit instruction. */
		}

		/* If the result is valid, exit the loop. */
		if (result >= 0) {
			break;
		}
	}

	printk_trace("BT: i = %d, parse_addr = %p, PC = %p, framesize = %d\n", i, parse_addr, PC, framesize); /**< Log backtrace progress. */

	framesize = result; /**< Set the frame size from the result. */

	/* If the scan limit is reached, report overflow error. */
	if (i == BT_SCAN_MAX_LIMIT) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. scope overflow\n"); /**< Log error on overflow. */
		return -1;
	}

	/* Process the stack frames by checking the stack instructions. */
	for (i = 0; parse_addr - i >= PC; i += 2) {
		/* Validate the address before processing the instruction. */
		if (backtrace_check_address(parse_addr - i) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr - i); /**< Log failed address. */
			return -1;
		}

		/* Check the previous 2-byte instruction for validity. */
		if (backtrace_check_address(parse_addr - i - 2) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr - i - 2); /**< Log failed address. */
			return -1;
		}

		ins16_l = *(uint16_t *) (parse_addr - i - 2); /**< Fetch the low 16-bit instruction. */
		ins16_h = *(uint16_t *) (parse_addr - i);	  /**< Fetch the high 16-bit instruction. */

		/* If the instruction is 4 bytes, process as 32-bit instruction. */
		if (insn_length(ins16_l) == 4) {
			ins32 = (ins16_h << 16) | ins16_l;			   /**< Combine into a 32-bit instruction. */
			temp = riscv_ins32_backtrace_stack_pop(ins32); /**< Process stack pop for 32-bit instruction. */
			i += 2;										   /**< Adjust the loop counter for 32-bit instruction. */
		} else {
			ins16 = ins16_h;							   /**< Use the high 16-bit instruction for 16-bit processing. */
			temp = riscv_ins16_backtrace_stack_pop(ins16); /**< Process stack pop for 16-bit instruction. */
		}

		/* If stack pop is valid, add the frame size. */
		if (temp >= 0) {
			printk_trace("BT: framesize add %d\n", temp); /**< Log the frame size addition. */
			framesize += temp;							  /**< Add to the total frame size. */
		}
	}

	printk_trace("BT: i = %d, parse_addr = %p, PC = %p, SP = %p, framesize = %d\n", i, parse_addr, PC, SP, framesize); /**< Log final backtrace details. */

	/* Check if the LR is valid again before updating SP and PC. */
	if (backtrace_check_address(LR) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR); /**< Log invalid LR. */
		return -1;
	}
	*pSP = SP + framesize;						 /**< Update stack pointer with the computed frame size. */
	offset = riscv_backtrace_find_lr_offset(LR); /**< Find the LR offset again. */
	*pPC = LR - offset;							 /**< Update program counter based on the LR and offset. */

	printk_trace("BT: *pSP = %p, offset = %d, *pPC = %p\n", *pSP, offset, *pPC); /**< Log the updated SP and PC. */

	return offset == 0 ? 1 : 0; /**< Return success if offset is 0, otherwise return 0. */
}

/**
 * @brief Perform a backtrace from the given program counter (PC), stack pointer (SP), and link register (LR).
 * 
 * This function tries to traverse the stack to generate a backtrace, logging the current PC at each level.
 * If the stack backtrace fails, it attempts to trace using the LR as a fallback.
 * 
 * @param PC Pointer to the program counter.
 * @param SP Pointer to the stack pointer.
 * @param LR Pointer to the link register (return address).
 * 
 * @return The number of backtrace levels found, or 0 if the backtrace failed.
 */
int backtrace(char *PC, long *SP, char *LR) {
	int level = 0;///< Backtrace level counter
	int ret;	  ///< Return value for backtrace_from_stack

	char *_PC = PC;///< Program counter (PC)
	long *_SP = SP;///< Stack pointer (SP)
	char *_LR = LR;///< Link register (LR)

	// Log the current program counter (PC)
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n", PC);

	// Traverse the stack and perform backtrace
	for (level = 1; level < BT_LEVEL_LIMIT; level++) {
		ret = backtrace_from_stack(&SP, &PC, &LR);///< Get the next backtrace level
		if (ret != 0) {
			break;///< Stop if backtrace fails
		}
	}

	/* If stack backtrace fails, try to trace using the link register (LR) */
	if (level == 1) {
		ret = riscv_backtrace_from_lr(&_SP, &_PC, _LR);///< Try backtrace from LR
		if (ret == 0) {
			for (; level < BT_LEVEL_LIMIT; level++) {
				ret = backtrace_from_stack(&SP, &PC, &LR);///< Continue stack backtrace if LR tracing succeeds
				if (ret != 0) {
					break;
				}
			}
		}
	}

	// Return the backtrace level, ensuring it's at least 0
	return level > 0 ? level : 0;///< Return the number of backtrace levels found
}

/**
 * @brief Dumps the current stack trace by fetching the program counter (PC), stack pointer (SP), and link register (LR).
 * 
 * This function retrieves the current values of the stack pointer, program counter, and link register,
 * then uses these to generate a backtrace. If either the stack pointer or program counter is invalid, it returns 0.
 * 
 * @return The backtrace level, or 0 if SP or PC is invalid.
 */
int dump_stack(void) {
	char *PC = NULL;///< Program counter (PC)
	long *SP = NULL;///< Stack pointer (SP)
	char *LR = NULL;///< Link register (LR)

	// Get the current stack pointer (SP)
	asm volatile("mv %0, sp\n"
				 : "=r"(SP));

	// Get the current program counter (PC)
	asm volatile("auipc %0, 0\n"
				 : "=r"(PC));

	// Get the return address (LR)
	asm volatile("mv %0, ra\n"
				 : "=r"(LR));

	// Check if stack pointer or program counter is invalid
	if (SP == NULL || PC == NULL) {
		return 0;///< Return 0 if SP or PC is invalid
	}

	// Return the backtrace level, ensuring it's at least 0
	return backtrace(PC, SP, LR);///< Call backtrace function and return the result
}
