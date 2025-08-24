/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <timer.h>

#include <log.h>

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
 * @brief ARM state macro definition
 *
 * This macro represents the ARM state in ARM architecture.
 */
#define ARM_STATE 0

/**
 * @brief THUMB state macro definition
 *
 * This macro represents the THUMB state in ARM architecture.
 */
#define THUMB_STATE 1

/**
 * @brief Macro to check if the program counter (PC) is in THUMB mode
 *
 * This macro checks whether the given program counter address (PC) points to the THUMB mode.
 * According to ARM architecture, if the least significant bit (LSB) of the program counter (PC) is 1, 
 * the instruction set is in THUMB mode.
 *
 * @param pc The program counter (typically the address of an instruction)
 * @return Returns a non-zero value if the address is in THUMB mode, otherwise returns 0.
 */
#define IS_THUMB_ADDR(pc) ((uint32_t) (pc) &0x1)

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
 * @brief Checks if a given Thumb instruction is a 32-bit Thumb instruction.
 *
 * This function examines the top 5 bits of the input instruction to determine
 * whether it corresponds to a specific set of Thumb instructions (0x1D, 0x1E, or 0x1F).
 * These values indicate a 32-bit Thumb instruction.
 *
 * @param ic A 16-bit uint32_teger representing the Thumb instruction to be checked.
 * 
 * @return Returns 1 if the instruction is a 32-bit Thumb instruction (i.e., its top 5 bits match
 *         0x1D, 0x1E, or 0x1F), otherwise returns 0.
 */
static int thumb_thumb32bit_code(uint16_t ic) {
	uint16_t op = (ic >> 11) & 0xFFFF;
	if (op == 0x1D || op == 0x1E || op == 0x1F)
		return 1;
	return 0;
}

/**
 * @brief Finds the offset for the Link Register (LR) based on the processor state.
 *
 * This function computes the offset for the Link Register (LR) based on the current processor
 * state (ARM or THUMB) and the instruction sequence at the return address. It checks for
 * specific branch instructions such as `bx` and `blx` and adjusts the LR offset accordingly.
 *
 * The function will also handle both 16-bit and 32-bit instructions based on the processor mode
 * and update the state of the processor.
 *
 * @param LR A pointer to the Link Register value (the return address).
 * @param state A pointer to a boolean representing the processor state. It can be either
 *              ARM_STATE or THUMB_STATE.
 * 
 * @return The offset to be applied to the LR, typically 2 or 4 bytes, depending on the 
 *         type of instruction found.
 */
static int find_lr_offset(char *LR, bool *state) {
	char *LR_fixed;///< Pointer to the adjusted Link Register address
	int offset;	   ///< Offset to be returned (2 or 4 bytes)

	// Check the processor state and determine the default offset
	if (*state == ARM_STATE) {
		offset = 4;///< ARM state typically uses 4-byte instructions
	} else if (*state == THUMB_STATE) {
		offset = 2;///< THUMB state typically uses 2-byte instructions
	}

	// Adjust LR address based on the processor's current mode
	LR_fixed = PC2ADDR(LR);///< Convert LR address to a fixed address for processing

	// Check if the address is valid and within text (code) segment
	if (backtrace_check_address(LR_fixed)) {
		uint16_t ins16 = *(uint16_t *) (LR_fixed - 2);///< 16-bit instruction at LR - 2
		uint32_t ins32 = *(uint32_t *) (LR_fixed - 4);///< 32-bit instruction at LR - 4

		// Check for "bx <register>" instruction (16-bit)
		if ((ins16 & 0xFF80) == 0x4700) {
			*state = !(*state);///< Toggle state between ARM and THUMB
			offset = 2;		   ///< "bx" instruction uses 2-byte offset
			printk_trace("BT: \tbx off=2\n");
		}
		// Check for "blx <register>" instruction (16-bit)
		else if ((ins16 & 0xFF80) == 0x4780) {
			*state = !(*state);///< Toggle state
			offset = 2;		   ///< "blx" instruction uses 2-byte offset
			printk_trace("BT: \tblx off=2\n");
		}
		// Check for "bx <register>" instruction (32-bit)
		else if ((ins32 & 0x0FFFFFF0) == 0x012FFF10) {
			*state = !(*state);///< Toggle state
			offset = 4;		   ///< "bx" instruction uses 4-byte offset
			printk_trace("BT: \tbx off=4\n");
		}
		// Check for "blx immediate" instruction (32-bit)
		else if ((ins32 & 0xFE000000) == 0xFA000000) {
			*state = !(*state);///< Toggle state
			offset = 4;		   ///< "blx immediate" instruction uses 4-byte offset
			printk_trace("BT: \tblx #imm off=4\n");
		}
		// Check for another form of "blx immediate" (32-bit)
		else if ((ins32 & 0xF800D000) == 0xF000C000) {
			*state = !(*state);///< Toggle state
			offset = 4;		   ///< "blx immediate" instruction uses 4-byte offset
			printk_trace("BT: \tblx #imm off=4\n");
		}
		// If state is THUMB and not a specific instruction, determine offset using helper function
		else if (*state == THUMB_STATE) {
			ins16 = *(uint16_t *) (LR_fixed - 4);			   ///< Fetch instruction at LR - 4
			offset = thumb_thumb32bit_code(ins16) == 1 ? 4 : 2;///< Determine offset based on instruction type
		}
	}

	// Log the final backtrace information with the calculated offset
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n", LR_fixed - offset);

	// Return the calculated offset (2 or 4)
	return offset;
}

/**
 * @brief Retrieves the next Thumb instruction from the given addresses.
 *
 * This function fetches the next instruction in the Thumb instruction set from two 16-bit
 * instruction addresses. It checks whether the instruction is a 32-bit Thumb-2 instruction 
 * (Thumb-32), and if so, combines the two 16-bit values to form a 32-bit instruction. It 
 * also updates the offset and returns the appropriate instruction.
 *
 * The function uses the `lsb` (least significant bit) flag to determine which part of the 
 * instruction to select when the instruction is not Thumb-32.
 *
 * @param error A pointer to an integer that will be set to `-1` in case of an error.
 * @param offset A pointer to the offset, which will be updated if a Thumb-32 instruction is found.
 * @param ins16_h_addr Address of the high part of the instruction (16 bits).
 * @param ins16_l_addr Address of the low part of the instruction (16 bits).
 * @param lsb Flag indicating whether to fetch the least significant bit of the instruction.
 * @param thumb32bit A pointer to an integer that will be set to `1` if a Thumb-32 instruction is detected, otherwise `0`.
 * 
 * @return Returns the 32-bit instruction if Thumb-32 is detected, otherwise returns the appropriate 16-bit instruction.
 */
static int thumb_get_next_inst(int *error, int *offset, char *ins16_h_addr, char *ins16_l_addr, int lsb, int *thumb32bit) {
	uint32_t ins32 = 0;	 ///< 32-bit instruction to be returned
	uint16_t ins16_l = 0;///< Lower 16 bits of the instruction
	uint16_t ins16_h = 0;///< Higher 16 bits of the instruction

	// Check if the low address is valid (points to valid code segment)
	if (backtrace_check_address(ins16_l_addr) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", ins16_l_addr);
		*error = -1;///< Set error flag to indicate invalid address
		return -1;	///< Return -1 to indicate error
	}
	ins16_l = *(uint16_t *) (ins16_l_addr);///< Fetch lower 16 bits of the instruction

	// Check if the high address is valid (points to valid code segment)
	if (backtrace_check_address(ins16_h_addr) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", ins16_h_addr);
		*error = -1;///< Set error flag to indicate invalid address
		return -1;	///< Return -1 to indicate error
	}
	ins16_h = *(uint16_t *) (ins16_h_addr);///< Fetch higher 16 bits of the instruction

	// Check if the low part of the instruction indicates a Thumb-32 instruction
	if (thumb_thumb32bit_code(ins16_l)) {
		/* Thumb-2 (Thumb-32) instruction: combine low and high parts */
		ins32 = (ins16_l << 16) | ins16_h;///< Combine low and high parts to form a 32-bit instruction
		*offset += 2;					  ///< Update offset (since it's a 32-bit instruction)
		*thumb32bit = 1;				  ///< Set thumb32bit flag to 1 indicating Thumb-32 instruction
	} else {
		*thumb32bit = 0;///< Set thumb32bit flag to 0, indicating it's not Thumb-32
		if (lsb) {
			ins32 = ins16_l;///< Use the low part of the instruction (16-bit)
		} else {
			ins32 = ins16_h;///< Use the high part of the instruction (16-bit)
		}
	}

	// Return the constructed 32-bit instruction (or a 16-bit instruction if not Thumb-32)
	return ins32;
}

/**
 * @brief Calculates the frame size for a `push` or `store-multiple` instruction that includes the link register (lr).
 *
 * This function processes a Thumb instruction to determine the frame size (number of registers pushed) 
 * based on the presence of the link register (`lr`). It checks both Thumb-2 (32-bit) and Thumb-1 (16-bit) 
 * instructions and computes the frame size accordingly.
 *
 * @param inst The instruction to analyze (32-bit Thumb instruction).
 * @param offset Pointer to an integer that will be set to `1` if a push or store-multiple instruction is found, 
 *               otherwise `0`.
 * @param thumb32bit A flag indicating whether the instruction is a 32-bit Thumb instruction (`1` if Thumb-32, 
 *                   `0` if Thumb-16).
 *
 * @return The number of registers pushed (frame size) or `-1` if no matching instruction is found.
 */
static int thumb_get_push_lr_ins_framesize(uint32_t inst, int *offset, int thumb32bit) {
	int framesize = -1;///< Default frame size if no matching instruction is found

	// Process Thumb-32 (Thumb-2) instruction
	if (thumb32bit) {
		// Check if the instruction is "stmdb sp!, {..., lr}"
		if ((inst & 0xFFFFF000) == 0xe92d4000) {
			// The instruction is "stmdb sp!, {..., lr}"
			printk_trace("BT: \tstmdb sp!, {..., lr}\n");
			framesize = __builtin_popcount(inst & 0xFFF);///< Count the number of registers in the mask (lower 12 bits)
			framesize++;								 ///< Add 1 for the lr register being stored
			*offset = 1;								 ///< Set offset to 1 (this is a 32-bit instruction)
		}
	} else {
		// Process Thumb-16 instructions
		// Check if the instruction is "push {..., lr, ...}"
		if ((inst & 0xFF00) == 0xB500) {
			// The instruction is "push {..., lr, ...}"
			printk_trace("BT: \tpush {..., lr, ... }\n");
			framesize = __builtin_popcount(inst & 0xFF) + 1;///< Count registers and add 1 for lr
			*offset = 1;									///< Set offset to 1 (this is a 16-bit instruction)
		}
		// Check if the instruction is "push { ... }"
		else if ((inst & 0xFF00) == 0xB400) {
			// The instruction is "push { ... }"
			printk_trace("BT: \tpush { ... }\n");
			framesize = __builtin_popcount(inst & 0xFF);///< Count the registers in the mask (lower 8 bits)
			*offset = 0;								///< Set offset to 0 (this is a 16-bit instruction without lr)
		}
	}

	// Print the instruction and its calculated frame size for debugging
	printk_trace("BT: inst:0x%x, framesize = %d\n", inst, framesize);

	return framesize;///< Return the frame size
}

/**
 * @brief Calculate the frame size based on the instruction and determine the stack adjustment.
 *
 * This function analyzes Thumb instructions to determine the frame size of a function prologue, 
 * based on push or stack adjustment operations. It handles both 16-bit and 32-bit Thumb instructions.
 *
 * @param inst The instruction to analyze (32-bit).
 * @param thumb32bit Flag indicating whether the instruction is a 32-bit Thumb instruction (1 for Thumb-32, 0 for Thumb-16).
 * 
 * @return The calculated frame size (number of registers pushed or stack adjusted), or -1 if no match is found.
 */
static int thumb_backtrace_stack_push(uint32_t inst, int thumb32bit) {
	uint32_t sub;		/**< Temporary variable to hold the immediate value for 'sub' instruction. */
	uint32_t shift;		/**< Temporary variable to hold the calculated shift value. */
	int framesize = -1; /**< Default frame size, indicating no matching instruction. */

	if (thumb32bit) {
		// Check if the instruction is a store-multiple decrement (stmdb sp!, {...})
		if ((inst & 0xFFFFF000) == 0xe92d4000) {
			printk_trace("BT: \tstmdb sp!, { ... }\n");
			framesize = __builtin_popcount(inst & 0xfff); /**< Count the number of registers to be stored. */
			framesize++;								  /**< Add 1 for the 'lr' register being stored. */
		}
		// Check if the instruction is a subtract (sub.w sp, sp, #imm)
		else if ((inst & 0xFBFF8F00) == 0xF1AD0D00) {
			printk_trace("BT: \tsub.w  sp, sp, #imm\n");
			sub = 128 + (inst & 0x7f); /**< Calculate the immediate value to subtract from sp. */
			shift = (inst >> 7) & 0x1; /**< Extract and calculate the shift value for the instruction. */
			shift += ((inst >> 12) & 0x7) << 1;
			shift += ((inst >> 26) & 0x1) << 4;
			framesize = sub << (30 - shift); /**< Calculate the frame size by applying the shift to the immediate. */
		}
		// Check if the instruction is a vector push for 64-bit registers (vpush {...} x64)
		else if ((inst & 0xffbf0f00) == 0xed2d0b00) {
			printk_trace("BT: \tvpush {...} x64\n");
			framesize = (inst & 0xff); /**< Frame size is determined by the lower byte of the instruction. */
		}
		// Check if the instruction is a vector push for 32-bit registers (vpush {...} x32)
		else if ((inst & 0xffbf0f00) == 0xed2d0a00) {
			printk_trace("BT: \tvpush {...} x32\n");
			framesize = (inst & 0xff); /**< Frame size is determined by the lower byte of the instruction. */
		}
	} else {
		// Check if the instruction is a push instruction involving the link register (push {..., lr})
		if ((inst & 0xff00) == 0xb500) {
			printk_trace("BT: \tpush {..., lr}\n");
			framesize = __builtin_popcount(inst & 0xff); /**< Count the registers being pushed, including lr. */
			framesize++;								 /**< Add 1 for the 'lr' register. */
		}
		// Check if the instruction is a subtract instruction (sub sp, sp, #imm)
		else if ((inst & 0xff80) == 0xb080) {
			printk_trace("BT: \tsub sp, sp, #imm\n");
			framesize = (inst & 0x7f); /**< Calculate the immediate value to subtract from sp. */
		}
	}

	// Print the instruction and its calculated frame size for debugging.
	printk_trace("BT: inst:0x%x, framesize = %d\n", inst, framesize);
	return framesize; /**< Return the calculated frame size. */
}

/**
 * @brief Perform a backtrace from the stack and retrieve the next return address and program counter (PC).
 *
 * This function traverses the stack to reconstruct a backtrace by parsing Thumb instructions and
 * calculating the correct stack pointer (SP), program counter (PC), and link register (LR). It 
 * uses the Thumb instruction set to decode stack frames and returns the updated SP, PC, and LR values.
 * 
 * @param pSP Pointer to the current stack pointer (SP).
 * @param pPC Pointer to the current program counter (PC).
 * @param pLR Pointer to the current link register (LR).
 * 
 * @return 
 * - 1: If the backtrace is successfully completed.
 * - 0: If the backtrace is successful but no LR was found.
 * - -1: If an error occurs (e.g., invalid address, overflow, or failed instruction parsing).
 */
static int thumb_backtrace_from_stack(int **pSP, char **pPC, char **pLR) {
	char *parse_addr = NULL;				/**< Temporary variable to hold address being parsed. */
	int *SP = *pSP;							/**< Current stack pointer (SP) value. */
	char *PC = PC2ADDR(*pPC);				/**< Converted program counter (PC) address. */
	char *LR = NULL;						/**< Temporary variable to hold the link register (LR). */
	int i, temp, framesize = 0, offset = 1; /**< Temporary variables for iteration, frame size, and offset calculation. */
	uint32_t ins32 = 0;						/**< 32-bit instruction fetched during the backtrace. */
	bool state = THUMB_STATE;				/**< Current state of the processor (THUMB_STATE). */

	// Loop to search for the first instruction that indicates the frame size.
	for (int i = 2; i < BT_SCAN_MAX_LIMIT; i += 2) {
		parse_addr = PC - i;																 /**< Calculate the address to fetch instructions from. */
		int error = 0;																		 /**< Error flag for instruction parsing. */
		int thumb32bit = 0;																	 /**< Flag to indicate whether the instruction is a 32-bit Thumb instruction. */
		ins32 = thumb_get_next_inst(&error, &i, parse_addr, parse_addr - 2, 0, &thumb32bit); /**< Fetch the next instruction. */
		if (error) {
			return -1; /**< Return -1 if an error occurs during instruction fetching. */
		}
		framesize = thumb_get_push_lr_ins_framesize(ins32, &offset, thumb32bit); /**< Get the frame size based on the instruction. */
		if (framesize >= 0) {
			break; /**< Exit loop if valid frame size is found. */
		}
	}

	printk_trace("BT: i = %d, parse_addr = %p, PC = %p, offset = %d, framesize = %d\n", i, parse_addr, PC, offset, framesize);

	// Check if the scan reached the maximum limit without finding a valid frame size.
	if (i == BT_SCAN_MAX_LIMIT) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. scope overflow\n");
		return -1;
	}

	// Process remaining instructions to calculate the total frame size.
	for (i = 2; parse_addr + i <= PC; i += 2) {
		int error = 0;																				 /**< Error flag for instruction parsing. */
		int thumb32bit = 0;																			 /**< Flag for 32-bit Thumb instruction. */
		ins32 = thumb_get_next_inst(&error, &i, parse_addr + i + 2, parse_addr + i, 1, &thumb32bit); /**< Fetch next instruction. */
		if (error) {
			return -1; /**< Return -1 if an error occurs. */
		}
		temp = thumb_backtrace_stack_push(ins32, thumb32bit); /**< Calculate the stack frame size for this instruction. */
		if (temp >= 0) {
			framesize += temp; /**< Add the calculated frame size to the total. */
		}
	}

	printk_trace("BT: i = %d, framesize = %d, SP = %p, offset = %d\n", i, framesize, SP, offset);

	// If offset is zero, use the provided link register (LR).
	if (offset == 0) {
		LR = *pLR;
	}

	// If no LR found, try to fetch it from the stack.
	if (!LR) {
		if (backtrace_check_address(SP + framesize - offset) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", SP + framesize - offset);
			return -1; /**< Return -1 if the LR address is invalid. */
		}
		LR = (char *) *(SP + framesize - offset); /**< Fetch LR from the calculated stack address. */
	}

	// Check if the LR address is valid.
	if (backtrace_check_address(LR) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR);
		return -1; /**< Return -1 if the LR is invalid. */
	}

	*pSP = SP + framesize;				 /**< Update the stack pointer (SP) based on the calculated frame size. */
	offset = find_lr_offset(LR, &state); /**< Find the offset for the link register (LR). */
	*pPC = LR - offset;					 /**< Calculate the program counter (PC) based on the LR and offset. */

	// Adjust for THUMB state if needed.
	if (state == THUMB_STATE) {
		MAKE_THUMB_ADDR(*pPC); /**< Make the program counter address compatible with THUMB state. */
	}

	printk_trace("BT: *pSP = %p, offset = %d, *pPC = %p, state=%d\n", *pSP, offset, *pPC, state);

	return offset == 0 ? 1 : 0; /**< Return 1 if no offset, else return 0. */
}

/**
 * @brief Analyzes an ARM instruction to determine the frame size and the offset for the link register (LR).
 * 
 * This function inspects the provided ARM instruction to detect specific types of instructions 
 * related to pushing the link register (LR) onto the stack. It calculates the size of the stack frame 
 * and determines the offset of the link register, if applicable.
 *
 * @param inst The ARM instruction to be analyzed (32-bit uint32_teger).
 * @param offset Pointer to an integer where the offset of LR will be stored.
 * 
 * @return The size of the stack frame in terms of the number of registers pushed, or -1 if no relevant instruction is found.
 */
static int arm_get_push_lr_ins_framesize(uint32_t inst, int *offset) {
	int framesize = -1; /**< Default return value when no matching instruction is found */

	// Check if the instruction is 'push {..., lr, ...}' (with optional PC)
	if ((inst & 0xFFFF4000) == 0xE92D4000) {
		/* push {..., lr, ... } */
		printk_trace("BT: \tpush {..., lr, ... }\n");

		// Check if the instruction also pushes the 'pc', set offset to 2 if true, else to 1 for just 'lr'
		*offset = (inst & 0x8000) == 0x8000 ? 2 : 1;

		// Count how many registers are pushed (count the 1s in the lower 16 bits)
		framesize = __builtin_popcount(inst & 0xFFFF);
	}
	// Check if the instruction is 'str lr [sp, #-4]!' (store lr at sp-4)
	else if (inst == 0xE52DE004) {
		/* str lr [sp, #-4]! */
		printk_trace("BT: \tstr lr [sp, #-4]!\n");
		*offset = 1;   /**< Only one register (lr) involved */
		framesize = 1; /**< Only one register is stored (lr) */
	}
	// Check if the instruction is a general 'push' instruction
	else if ((inst & 0xFFFF0000) == 0xE92D0000) {
		/* push {.....} */
		printk_trace("BT: \tpush {.....}\n");
		*offset = 0;								   /**< No special handling for link register here */
		framesize = __builtin_popcount(inst & 0xFFFF); /**< Count the number of registers pushed */
	}

	// Debug trace of the instruction and calculated frame size
	printk_trace("BT: inst = 0x%x, framesize = %d\n", inst, framesize);

	return framesize; /**< Return the calculated frame size */
}

/**
 * @brief Analyzes an ARM instruction to determine the frame size for stack operations.
 * 
 * This function inspects the provided ARM instruction to determine the size of the stack frame 
 * based on common instructions such as `sub sp, sp, #imm`, `push`, `vpush`, and `str xxx, [sp, #-4]!`.
 * It returns the size of the stack frame in terms of the number of words (4-byte units) affected by the instruction.
 *
 * @param inst The ARM instruction to be analyzed (32-bit uint32_teger).
 * 
 * @return The size of the stack frame in terms of the number of 4-byte words, or -1 if no relevant instruction is found.
 */
static int arm_backtrace_stack_push(uint32_t inst) {
	uint32_t sub;		/**< Temporary variable for the immediate value */
	uint32_t shift;		/**< Temporary variable for the shift value */
	int framesize = -1; /**< Default return value when no matching instruction is found */

	// Check if the instruction is 'sub sp, sp, #imm' (subtract an immediate from sp)
	if ((inst & 0x0FFFF000) == 0x024DD000) {
		/* sub sp, sp, #imm */
		printk_trace("BT: \tsub sp, sp, #imm\n");

		sub = inst & 0xFF;		   /**< Extract the immediate value */
		shift = (inst >> 8) & 0xF; /**< Extract the shift value */

		// Adjust the immediate value based on the shift
		if (shift != 0) {
			shift = 32 - 2 * shift; /**< Convert shift into bitwise adjustment */
			sub = sub << shift;		/**< Apply the shift to the immediate value */
		}

		framesize = sub / 4; /**< Calculate the frame size in terms of 4-byte words */
	}
	// Check if the instruction is a 'push {...}' (push registers onto the stack)
	else if ((inst & 0x0FFF0000) == 0x092D0000) {
		/* push {...} */
		printk_trace("BT: \tpush {...}\n");
		framesize = __builtin_popcount(inst & 0xFFFF); /**< Count the number of registers pushed */
	}
	// Check if the instruction is a 'vpush {...}' (push vector registers onto the stack)
	else if ((inst & 0x0FBF0F00) == 0x0D2D0B00) {
		/* vpush {...} */
		printk_trace("BT: \tvpush {...}\n");
		framesize = inst & 0xFF; /**< Set frame size to the number of vector registers pushed */
	}
	// Check if the instruction is 'str xxx, [sp, #-4]!' (store register at sp-4)
	else if ((inst & 0xFFFF0FFF) == 0xE52D0004) {
		/* str xxx, [sp, #-4]! */
		printk_trace("BT: \tstr xxx, [sp, #-4]!\n");
		framesize = 1; /**< Only one register is stored (frame size = 1 word) */
	}

	// Debug trace of the instruction and calculated frame size
	printk_trace("BT: inst = 0x%x, framesize = %d\n", inst, framesize);

	return framesize; /**< Return the calculated frame size */
}

/**
 * @brief Performs backtrace from the stack to find the return address and program counter.
 * 
 * This function attempts to deduce the return address (LR) and program counter (PC) by analyzing 
 * the stack and the instructions around it. It parses the stack to find the correct frame size, 
 * validates the text addresses, and adjusts the program counter based on the instruction state 
 * (ARM/Thumb).
 *
 * @param pSP Pointer to the current stack pointer (SP).
 * @param pPC Pointer to the current program counter (PC).
 * @param pLR Pointer to the link register (LR).
 * 
 * @return 1 if a valid return address is found, 0 if the backtrace is incomplete, and -1 if an error occurs.
 */
static int arm_bakctrace_from_stack(int **pSP, char **pPC, char **pLR) {
	int *SP = *pSP;									 /**< Current stack pointer */
	char *PC = *pPC;								 /**< Current program counter */
	char *LR = NULL;								 /**< Link register (LR) will be determined later */
	char *parse_addr = NULL;						 /**< Address used for parsing instructions */
	int i, temp, framesize, offset = 0, swi_num = 0; /**< Temporary variables for backtrace calculations */
	uint32_t ins32 = 0;								 /**< Holds instruction value */
	bool state = ARM_STATE;							 /**< CPU state (ARM or Thumb) */

	// Scan for a valid frame in the first few instructions
	for (i = 4; i < BT_SCAN_MAX_LIMIT; i += 4) {
		parse_addr = PC - i;
		if (backtrace_check_address(parse_addr) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", parse_addr);
			return -1; /**< Invalid address encountered */
		}

		ins32 = *(long *) parse_addr;							   /**< Fetch the instruction at the parse address */
		framesize = arm_get_push_lr_ins_framesize(ins32, &offset); /**< Determine the frame size and offset */
		if (framesize >= 0) {
			break; /**< Found a valid instruction with frame size */
		}
	}

	// Trace information about the found instruction and frame size
	printk_trace("BT: i = %d, parse_addr = %p, PC = %p, offset = %d, framesize = %d\n", i, parse_addr, PC, offset, framesize);

	if (i == BT_SCAN_MAX_LIMIT) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. scope overflow\n");
		return -1; /**< Reached the scan limit without finding a valid frame */
	}

	// Scan the stack for additional instructions and calculate frame size
	for (i = 4; parse_addr + i < PC; i += 4) {
		if (backtrace_check_address(parse_addr + i) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", parse_addr + i);
			return -1; /**< Invalid address encountered */
		}

		ins32 = *(uint64_t *) (parse_addr + i); /**< Fetch the instruction at the next address */
		temp = arm_backtrace_stack_push(ins32); /**< Calculate the additional frame size */
		if (temp >= 0) {
			framesize += temp; /**< Add the frame size to the total */
		}

		if (ins32 == 0xef000000) { /**< SWI (Software Interrupt) detected */
			swi_num++;			   /**< Count the number of software interrupts */
		}
	}

	// Check if previous instruction is valid and update frame size
	if (backtrace_check_address(parse_addr - 4)) {
		ins32 = *(uint64_t *) (parse_addr - 4); /**< Get the previous instruction */
	} else {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr - 4);
		return -1; /**< Invalid address encountered */
	}

	temp = arm_backtrace_stack_push(ins32); /**< Update frame size based on the previous instruction */
	if (temp >= 0) {
		framesize += temp;
		offset += temp;
	}

	framesize -= swi_num; /**< Adjust the frame size by subtracting the SWI count */

	// Determine the link register (LR) based on the offset
	if (offset == 0) {
		LR = *pLR; /**< Use the provided LR if no offset was found */
	}

	if (LR == NULL) {
		if (backtrace_check_address(SP + framesize - offset)) {
			LR = (char *) *(SP + framesize - offset); /**< Calculate LR from stack if not provided */
		} else {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", SP + framesize - offset);
			return -1; /**< Invalid address for LR calculation */
		}
	}

	// Check the validity of the LR address
	if (backtrace_check_address(LR) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR);
		return -1; /**< Invalid LR address */
	}

	*pSP = SP + framesize;				 /**< Update the stack pointer */
	offset = find_lr_offset(LR, &state); /**< Find the offset of the LR based on the state */
	*pPC = LR - offset;					 /**< Calculate the program counter (PC) based on LR and offset */

	// Adjust the program counter if in Thumb mode
	if (state == THUMB_STATE) {
		MAKE_THUMB_ADDR(*pPC); /**< Adjust PC for Thumb instructions */
	}

	// Trace the final stack pointer, program counter, and frame size
	printk_trace("BT: *pSP = %p, offset = %d, *pPC = %p, framesize = %d, state = %d\n", *pSP, offset, *pPC, framesize, state);

	return offset == 0 ? 1 : 0; /**< Return 1 if backtrace is successful, else 0 or -1 */
}

/**
 * @brief Backtrace from stack based on program counter (PC) state.
 * 
 * This function performs a backtrace by checking if the current code is running in ARM or Thumb 
 * state and calls the appropriate backtrace function accordingly. If the program counter (PC) 
 * address is invalid, it returns an error.
 *
 * @param pSP Pointer to the stack pointer.
 * @param pPC Pointer to the program counter.
 * @param pLR Pointer to the link register.
 * 
 * @return 0 if backtrace was successful, -1 if the PC is invalid or an error occurred.
 */
static int backtrace_from_stack(int **pSP, char **pPC, char **pLR) {
	// Check if the provided program counter (PC) address is valid
	if (backtrace_check_address(*pPC) == 0) {
		return -1;// Invalid address, return error
	}

	// Check if the address is in Thumb state (16-bit instruction set)
	if (IS_THUMB_ADDR(*pPC)) {
		return thumb_backtrace_from_stack(pSP, pPC, pLR);// Perform Thumb-specific backtrace
	} else {
		return arm_bakctrace_from_stack(pSP, pPC, pLR);// Perform ARM-specific backtrace
	}
}

/**
 * @brief Pop a frame from the stack for Thumb instructions.
 *
 * This function processes a Thumb instruction to determine the frame size 
 * and updates the stack pointer change flag. It distinguishes between normal 
 * "pop" and vector "vpop" instructions.
 * 
 * @param inst The Thumb instruction to analyze.
 * @param sp_change Pointer to a flag that will be set to 1 if the stack pointer changes.
 * @param thumb32bit Flag indicating if the instruction is Thumb-32 bit (true) or not (false).
 * 
 * @return The size of the frame to pop, or -1 if no valid "pop" instruction is found.
 */
static int thumb_backtrace_stack_pop(uint32_t inst, uint8_t *sp_change, int thumb32bit) {
	int framesize = -1; /**< Initialize frame size to -1 (invalid) */

	// Check for non-Thumb-32 bit instruction
	if (!thumb32bit) {
		// Check for "pop {...}" instruction pattern
		if ((inst & 0xff00) == 0xbc00) {
			printk_trace("BT: \tpop {...}\n");				/**< Trace log for pop instruction */
			framesize = __builtin_popcount((uint8_t) inst); /**< Count the number of registers popped */
			*sp_change = 1;									/**< Set stack pointer change flag */
		}
	} else {
		// Check for "vpop {...}" instruction pattern
		if ((inst & 0xffbf0f00) == 0xecbd0b00) {
			printk_trace("BT: \tvpop {...}\n"); /**< Trace log for vpop instruction */
			framesize = inst & 0xFF;			/**< Get the frame size from the instruction */
			*sp_change = 1;						/**< Set stack pointer change flag */
		}
	}

	printk_trace("BT: inst = 0x%x, framesize = %d\n", inst, framesize); /**< Log the instruction and frame size */
	return framesize;													/**< Return the calculated frame size (or -1 if invalid) */
}

/**
 * @brief Process the return instruction for Thumb mode and determine the frame size.
 *
 * This function checks if the current instruction is a return instruction, such as 
 * "bx lr", which is used for returning from functions in ARM architecture. 
 * It specifically handles the case for Thumb mode instructions and sets the 
 * frame size appropriately.
 * 
 * @param inst The Thumb instruction to analyze.
 * @param thumb32bit Flag indicating if the instruction is Thumb-32 bit (true) or not (false).
 * 
 * @return 0 if the return instruction is detected, otherwise the frame size (or -1 if not detected).
 */
static int thumb_backtrace_return_pop(uint32_t inst, int thumb32bit) {
	int framesize = -1; /**< Initialize frame size to -1 (invalid) */

	// Check for non-Thumb-32 bit instruction
	if (!thumb32bit) {
		/* Check for "bx lr" instruction (branch to link register) */
		printk_trace("BT: \tbx lr\n"); /**< Trace log for bx lr instruction */
		if ((inst & 0xFFFF) == 0x4770) {
			return 0; /**< Return 0 for a valid return instruction */
		}
	}

	printk_trace("BT: inst = 0x%x, framesize = %d\n", inst, framesize); /**< Log the instruction and frame size */
	return framesize;													/**< Return the frame size (or -1 if not a valid return instruction) */
}

/**
 * @brief Calculate the frame size for a "push" instruction in Thumb mode.
 *
 * This function checks if the instruction is a "push" instruction or a "sub sp" instruction 
 * in Thumb-32 bit mode. It determines the frame size based on the instruction and sets 
 * the jump flag if needed.
 * 
 * @param inst The Thumb instruction to analyze.
 * @param jump Pointer to a flag that will be set to 1 if the instruction is a "push".
 * @param thumb32bit Flag indicating if the instruction is Thumb-32 bit (true) or not (false).
 * 
 * @return The calculated frame size, or -1 if the instruction does not match "push" or "sub sp".
 */
static int thumb_get_push_ins_framesize(uint32_t inst, uint8_t *jump, int thumb32bit) {
	int framesize = -1; /**< Initialize frame size to -1 (invalid) */

	// Check for Thumb-32 bit instructions
	if (thumb32bit) {
		// Check for "push {...}" instruction pattern
		if ((inst & 0xff00) == 0xb400) {
			printk_trace("BT: \tpush {...}\n");				/**< Trace log for push instruction */
			framesize = __builtin_popcount((uint8_t) inst); /**< Count the number of registers pushed */
			*jump = 1;										/**< Set jump flag to 1 */
		}
		// Check for "sub sp, #immed" instruction pattern
		else if ((inst & 0xff80) == 0xb080) {
			printk_trace("BT: \tsub sp, #immed\n"); /**< Trace log for sub sp instruction */
			framesize = (inst & 0x7f);				/**< Get immediate value to determine the frame size */
		}
	}

	printk_trace("BT: inst:0x%x, framesize = %d\n", inst, framesize); /**< Log the instruction and frame size */
	return framesize;												  /**< Return the calculated frame size (or -1 if invalid) */
}

/**
 * @brief Perform a backtrace from the Link Register (LR) in Thumb mode.
 * 
 * This function extracts the frame size from the LR and updates the SP and PC 
 * based on the Thumb instruction set. It scans the instructions, identifies 
 * the backtrace points, and handles stack frame adjustments.
 * 
 * @param pSP Pointer to the stack pointer.
 * @param pPC Pointer to the program counter.
 * @param LR The link register address to backtrace from.
 * @return 1 if backtrace is successful, 0 if the frame is not valid, -1 on error.
 */
static int thumb_backtrace_from_lr(int **pSP, char **pPC, char *LR) {
	int *SP = *pSP;								   /**< Current stack pointer */
	char *PC = PC2ADDR(*pPC);					   /**< Program counter address converted to proper format */
	char *parse_addr = NULL;					   /**< Address used for parsing instructions */
	int i, temp = 0;							   /**< Loop counter and temporary value for instruction processing */
	uint32_t ins32 = 0, framesize = 0, offset = 0; /**< Instruction, frame size, and LR offset */
	uint8_t sp_change = 0, jump = 0;			   /**< Flags indicating stack pointer change and jump */

	bool state = THUMB_STATE; /**< Current CPU state (Thumb or ARM) */

	// Check if the program counter is a valid text address
	if (backtrace_check_address(PC) == 0) {
		// Check if the link register is a valid address
		if (backtrace_check_address(LR) == 0) {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR); /**< Log invalid LR address */
			return -1;														   /**< Return error on invalid LR */
		}
		// Find the LR offset and adjust the program counter
		offset = find_lr_offset(LR, &state);
		PC = LR - offset;			/**< Adjust program counter based on LR offset */
		*pPC = PC;					/**< Update PC pointer */
		return offset == 0 ? 1 : 0; /**< Return success if offset is 0, otherwise return failure */
	}

	// Start scanning instructions to perform backtrace
	for (i = 2; i < BT_SCAN_MAX_LIMIT; i += 2) {
		parse_addr = PC + i; /**< Set the address to parse the instruction */
		int error = 0;		 /**< Error flag for instruction fetching */
		int thumb32bit = 0;	 /**< Flag for Thumb-32 bit mode */

		// Get the next Thumb instruction
		ins32 = thumb_get_next_inst(&error, &i, PC + i + 2, PC + i, 1, &thumb32bit);
		if (error) {
			return -1; /**< Return error if instruction fetching fails */
		}

		printk_trace("BT: parse_addr = 0x%x, i = %d\n", parse_addr, i); /**< Trace parsed address */

		// Check if it's a stack pop operation
		temp = thumb_backtrace_stack_pop(ins32, &sp_change, thumb32bit);
		if (temp >= 0) {
			continue; /**< Continue scanning if it's a valid stack pop */
		}

		// Check if it's a return pop operation
		temp = thumb_backtrace_return_pop(ins32, thumb32bit);
		if (temp >= 0) {
			framesize += temp; /**< Add frame size for return pop */
			break;			   /**< Break after finding the return pop */
		}
	}

	// If scan limit is exceeded, return an error
	if (i == BT_SCAN_MAX_LIMIT) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. scope overflow\n"); /**< Log failure */
		return -1;
	}

	printk_trace("BT: parse_addr = 0x%08x, framesize = %d, sp_change = %d\n", parse_addr, framesize, sp_change); /**< Log framesize and SP change status */

	// If the stack pointer has changed, scan for push instructions
	if (sp_change) {
		for (i = 2; i < BT_SCAN_MAX_LIMIT; i += 2) {
			parse_addr = PC - i; /**< Adjust the parse address for backwards scanning */
			int error = 0;
			int thumb32bit = 0;

			// Get the next Thumb instruction while scanning backwards
			ins32 = thumb_get_next_inst(&error, &i, parse_addr, parse_addr - 2, 0, &thumb32bit);
			if (error) {
				return -1; /**< Return error if instruction fetching fails */
			}

			// Check if it's a push instruction and adjust framesize
			temp = thumb_get_push_ins_framesize(ins32, &jump, thumb32bit);
			if (temp >= 0) {
				framesize += temp; /**< Add to framesize for push instruction */
			}
			if (jump > 0) {
				break; /**< Break if a jump is encountered */
			}
		}
	}

	// Check if the LR is still valid
	if (backtrace_check_address(LR) == 0) {
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR); /**< Log invalid LR */
		return -1;														   /**< Return error if LR is invalid */
	}

	// Update the stack pointer and program counter
	*pSP = SP + framesize;				 /**< Update stack pointer based on framesize */
	offset = find_lr_offset(LR, &state); /**< Find LR offset */
	*pPC = LR - offset;					 /**< Update program counter */

	// Adjust program counter if in Thumb state
	if (state == THUMB_STATE) {
		MAKE_THUMB_ADDR(*pPC); /**< Adjust PC to Thumb address format */
	}

	printk_trace("BT: *pSP = %p, offset = %d, *pPC = %p, framesize = %d, state=%d\n", *pSP, offset, *pPC, framesize, state); /**< Trace final backtrace information */

	return offset == 0 ? 1 : 0; /**< Return success or failure based on LR offset */
}

/**
 * @brief Analyze the ARM instruction to determine the frame size for backtrace.
 * 
 * This function interprets an ARM instruction and determines the frame size based on the 
 * instruction type. It handles the following instructions:
 * - `pop {..., pc}`: Pops the program counter (pc) from the stack.
 * - `bx lr`: Branches to the link register (lr), effectively returning from a function.
 * - `ldr pc, [sp], #4`: Loads the program counter (pc) from the stack and updates the stack pointer.
 *
 * @param inst The ARM instruction to analyze.
 * 
 * @return The size of the frame affected by the instruction.
 */
static int arm_backtrace_return_pop(uint32_t inst) {
	int framesize = -1; /**< Default frame size is -1 (unknown) */

	if ((inst & 0x0FFF8000) == 0x08BD8000) { /**< Check for 'pop {..., pc}' instruction */
		printk_trace("BT: \tpop {..., pc}\n");
		framesize = __builtin_popcount(inst & 0xFFFF); /**< Count the number of registers popped */
	} else if ((inst & 0x0FFFFFFF) == 0x012FFF1E) {	   /**< Check for 'bx lr' instruction */
		printk_trace("BT: \tbx lr\n");
		framesize = 0;								/**< No frame size change for 'bx lr' */
	} else if ((inst & 0x0FFFFFFF) == 0x049DF004) { /**< Check for 'ldr pc, [sp], #4' instruction */
		printk_trace("BT: \tldr pc, [sp], #4\n");
		framesize = 1; /**< Frame size is 1 for 'ldr pc, [sp], #4' */
	}

	printk_trace("BT: inst = 0x%x, framesize = %d\n", inst, framesize); /**< Log instruction and frame size */
	return framesize;													/**< Return the computed frame size */
}

/**
 * @brief Analyze the ARM instruction to determine the frame size for stack pop operations.
 * 
 * This function interprets an ARM instruction and determines the frame size based on the 
 * instruction type. It handles the following instructions:
 * - `add sp, sp, #imm`: Adds an immediate value to the stack pointer (sp).
 * - `pop {...}`: Pops a set of registers from the stack.
 * - `vpop {...}`: Pops a set of vector registers from the stack.
 * - `ldr xxx, [sp], #4`: Loads a value from the stack into a register and updates the stack pointer.
 *
 * @param inst The ARM instruction to analyze.
 * 
 * @return The size of the frame affected by the instruction.
 */
static int arm_backtrace_stack_pop(uint32_t inst) {
	uint32_t add;		/**< Immediate value to be added to the stack pointer */
	uint32_t shift;		/**< Shift value for scaling the immediate */
	int framesize = -1; /**< Default frame size is -1 (unknown) */

	if ((inst & 0x0FFFF000) == 0x028DD000) { /**< Check for 'add sp, sp, #imm' instruction */
		printk_trace("BT: \tadd sp, sp, #imm\n");
		add = inst & 0xFF;		   /**< Extract the immediate value */
		shift = (inst >> 8) & 0xF; /**< Extract the shift value */
		if (shift != 0) {		   /**< Apply shift if non-zero */
			shift = 32 - 2 * shift;
			add = add << shift;
		}
		framesize = add / 4;						/**< Frame size is the immediate value divided by 4 (stack units) */
	} else if ((inst & 0x0FFF0000) == 0x08BD0000) { /**< Check for 'pop {...}' instruction */
		printk_trace("BT: \tpop {...}\n");
		framesize = __builtin_popcount(inst & 0xFFFF); /**< Count the number of registers popped */
	} else if ((inst & 0x0FBF0F00) == 0x0CBD0B00) {	   /**< Check for 'vpop {...}' instruction */
		printk_trace("BT: \tvpop {...}\n");
		framesize = inst & 0xFF;					/**< Frame size is determined by the immediate value (vector registers) */
	} else if ((inst & 0x0FFF0FFF) == 0x049D0004) { /**< Check for 'ldr xxx, [sp], #4' instruction */
		printk_trace("BT: \tldr xxx, [sp], #4\n");
		framesize = 1; /**< Frame size is 1 (single word loaded) */
	}

	printk_trace("BT: inst = 0x%x, framesize = %d\n", inst, framesize); /**< Log the instruction and computed frame size */
	return framesize;													/**< Return the computed frame size */
}

/**
 * @brief Perform backtrace from the link register (LR) to determine the stack pointer (SP) and program counter (PC).
 * 
 * This function performs a backtrace from the provided LR (link register) to locate the stack pointer 
 * and program counter of the current function. It checks the validity of addresses, scans instructions 
 * to compute the stack frame size, and adjusts the program counter based on the ARM instruction set state 
 * (ARM or Thumb).
 *
 * @param pSP A pointer to the stack pointer (SP) to be updated.
 * @param pPC A pointer to the program counter (PC) to be updated.
 * @param LR The link register value from which the backtrace will begin.
 * 
 * @return 1 if backtrace is successful with no offset, 0 if backtrace is successful with offset, 
 *         or -1 if there is an error during the process.
 */
static int arm_backtrace_from_lr(int **pSP, char **pPC, char *LR) {
	int *SP = *pSP;						/**< Current stack pointer */
	char *PC = *pPC;					/**< Current program counter */
	char *parse_addr = NULL;			/**< Temporary address used for parsing instructions */
	int i, temp, framesize = 0, offset; /**< Loop index, temporary storage, and frame size */
	uint32_t ins32;						/**< 32-bit instruction */
	bool state = ARM_STATE;				/**< ARM state (ARM or Thumb) */

	if (backtrace_check_address(PC) == 0) {		/**< Check if PC is a valid address */
		if (backtrace_check_address(LR) == 0) { /**< Check if LR is a valid address */
			printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR);
			return -1; /**< Invalid LR address */
		}
		offset = find_lr_offset(LR, &state); /**< Find the offset of LR */
		PC = LR - offset;					 /**< Set PC to LR minus offset */
		*pPC = PC;							 /**< Update the program counter pointer */
		return offset == 0 ? 1 : 0;			 /**< Return success or failure based on the offset */
	}

	for (i = 0; i < BT_SCAN_MAX_LIMIT; i += 4) { /**< Scan instructions up to a maximum limit */
		parse_addr = PC + i;
		if (backtrace_check_address(parse_addr)) {
			ins32 = *(uint64_t *) parse_addr; /**< Read instruction at current address */
		} else {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", parse_addr);
			return -1; /**< Invalid instruction address */
		}
		framesize = arm_backtrace_return_pop(ins32); /**< Analyze instruction for stack frame size */
		if (framesize >= 0) {						 /**< Frame size found, break loop */
			break;
		}

		temp = arm_get_push_lr_ins_framesize(ins32, &offset); /**< Check for push lr instruction */
		if (temp >= 0) {									  /**< Valid push lr instruction found */
			framesize = 0;									  /**< No frame size adjustment needed */
			break;
		}
	}

	if (i == BT_SCAN_MAX_LIMIT) { /**< If the maximum scan limit is reached, fail the backtrace */
		printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. scope overflow\n");
		return -1;
	}

	for (i = 4; parse_addr - i >= PC; i += 4) { /**< Scan previous instructions to adjust frame size */
		if (backtrace_check_address(parse_addr - i)) {
			ins32 = *(uint64_t *) (parse_addr - i); /**< Read previous instruction */
		} else {
			printk(LOG_LEVEL_BACKTRACE, "backtrace: failed. addr 0x%08x\n", parse_addr - i);
			return -1; /**< Invalid address during scan */
		}
		temp = arm_backtrace_stack_pop(ins32); /**< Analyze instruction for stack pop size */
		if (temp >= 0) {					   /**< Valid stack pop found, update frame size */
			framesize += temp;
		}
	}

	if (backtrace_check_address(LR) == 0) { /**< Check if LR is a valid address again */
		printk(LOG_LEVEL_BACKTRACE, "backtrace: invalid lr 0x%08x\n", LR);
		return -1; /**< Invalid LR address */
	}
	*pSP = SP + framesize;				 /**< Update the stack pointer based on the calculated frame size */
	offset = find_lr_offset(LR, &state); /**< Recalculate LR offset */
	*pPC = LR - offset;					 /**< Update program counter (PC) based on LR and offset */

	if (state == THUMB_STATE) { /**< If the state is Thumb, adjust the PC address for Thumb mode */
		MAKE_THUMB_ADDR(*pPC);
	}

	printk_trace("BT: *pSP = %p, offset = %d, *pPC = %p, framesize = %d, state = %d\n", *pSP, offset, *pPC, framesize, state);

	return offset == 0 ? 1 : 0; /**< Return 1 for success with no offset, 0 for success with offset */
}

/**
 * @brief Perform a backtrace from the Link Register (LR).
 * 
 * This function determines whether the current address is in Thumb mode or ARM mode
 * and calls the appropriate backtrace function based on the result.
 * 
 * @param pSP Pointer to the stack pointer.
 * @param pPC Pointer to the program counter.
 * @param LR The link register address to backtrace from.
 * @return 1 if backtrace is successful, 0 if the frame is not valid, -1 on error.
 */
static int backtrace_from_lr(int **pSP, char **pPC, char *LR) {
	// Check if the current program counter is in Thumb mode
	if (IS_THUMB_ADDR(*pPC)) {
		return thumb_backtrace_from_lr(pSP, pPC, LR); /**< Call Thumb backtrace function */
	} else {
		return arm_backtrace_from_lr(pSP, pPC, LR); /**< Call ARM backtrace function */
	}
}

/**
 * @brief Perform a backtrace to find the call stack.
 *
 * This function attempts to walk the call stack by analyzing the program counter (PC),
 * stack pointer (SP), and link register (LR) to determine the sequence of function calls 
 * leading to the current execution point.
 * 
 * The backtrace process works by using the stack and link register to trace the call hierarchy,
 * logging each level of the backtrace, and attempting recovery from any errors.
 * If the backtrace from the stack fails, it will attempt to trace using the link register (LR).
 * 
 * @param PC The current program counter (PC), typically pointing to the instruction where the backtrace starts.
 * @param SP The current stack pointer (SP), pointing to the top of the stack.
 * @param LR The link register (LR), used to store return addresses for function calls.
 * 
 * @return The number of backtrace levels successfully traversed. 
 *         Returns 0 if no valid backtrace could be performed.
 */
int backtrace(char *PC, long *SP, char *LR) {
	int level = 0;///< Backtrace level counter
	int ret;	  ///< Return value for backtrace_from_stack

	/* Backup PC, SP, LR for backtrace_from_lr */
	char *_PC = PC;///< Program counter (PC)
	long *_SP = SP;///< Stack pointer (SP)
	char *_LR = LR;///< Link register (LR)

	// Log the current program counter (PC)
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n", PC);

	// Traverse the stack and perform backtrace
	for (level = 1; level < BT_LEVEL_LIMIT; level++) {
		ret = backtrace_from_stack((int **) &SP, &PC, &LR);///< Get the next backtrace level
		if (ret != 0) {
			break;///< Stop if backtrace fails
		}
	}

	/* If stack backtrace fails, try to trace using the link register (LR) */
	if (level == 1) {
		ret = backtrace_from_lr((int **) &_SP, &_PC, _LR);
		if (ret == 0) {
			for (; level < BT_LEVEL_LIMIT; level++) {
				ret = backtrace_from_stack((int **) &SP, &PC, &LR);
				if (ret != 0) {
					break;
				}
			}
		}
	}

	// Return the backtrace level, ensuring it's at least 0
	return level > 0 ? level : 0;
}

/**
 * @brief Dumps the current stack state and performs a backtrace.
 *
 * This function captures the current program counter (PC), stack pointer (SP),
 * link register (LR), and current processor status register (CPSR). It uses inline assembly
 * to obtain these values, then performs a backtrace to provide insight into the function call stack.
 * 
 * The function also checks if the processor is in THUMB mode based on the CPSR state and adjusts
 * the program counter (PC) accordingly. If the program counter or stack pointer is invalid, 
 * the function will return early with a status of 0.
 *
 * @return The result of the backtrace function, representing the number of successfully 
 *         traced backtrace levels, or 0 if the stack pointer (SP) or program counter (PC) is invalid.
 */
int dump_stack(void) {
	char *PC = NULL;///< Program counter (PC)
	long *SP = NULL;///< Stack pointer (SP)
	char *LR = NULL;///< Link register (LR)
	int CPSR = 0;

	// Inline assembly to get the current stack pointer (SP), program counter (PC), link register (LR),
	// and current processor status register (CPSR).
	asm volatile("mov %0, sp\r\n"
				 : "=r"(SP));
	asm volatile("mov %0, pc\r\n"
				 : "=r"(PC));
	asm volatile("mov %0, lr\r\n"
				 : "=r"(LR));
	asm volatile("mrs %0, CPSR\r\n"
				 : "=r"(CPSR));

	/* CPSR Thumb state bit. bit[5] */
	if (CPSR & 0x20) {
		MAKE_THUMB_ADDR(PC);///< Adjust PC if in THUMB mode.
	}

	// Check if stack pointer or program counter is invalid
	if (SP == NULL || PC == NULL) {
		return 0;///< Return 0 if SP or PC is invalid
	}

	// Perform the backtrace using the collected values for PC, SP, and LR
	return backtrace(PC, SP, LR);
}
