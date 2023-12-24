/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

uint32_t rvbar0 = 0x017000a0;
uint32_t start_addr = 0x28000;

/*
 * First, it loads the value of rvbar0 into register r1, and the value of startAddr into register r0.
 * Then, it stores the value in register r0 to the address pointed to by r1,
 * which means storing startAddr to the MMIO mapped RVBAR[0] register.
 *
 * After that, it uses a data synchronization barrier (dsb) to ensure memory access completion,
 * and an instruction synchronization barrier (isb) to ensure correct instruction execution order and prevent instruction reordering.
 * Next, it reads the value of Register Maintenance Register (RMR) from co-processor CP15 into register r0,
 * performs a bitwise OR operation between the value in register r0 and 0x3, setting the lowest two bits to 1,
 * requesting a reset in AArch64 mode. Then, it writes the modified value to the RMR register in co-processor CP15,
 * requesting a reset. It uses the instruction synchronization barrier again afterwards.
 
 * Finally, it executes the Wait For Interrupt (WFI) instruction,
 * waiting for an external event to wake up the system. It jumps back to the WFI instruction to continue waiting using an infinite loop.
 */
void rmr_switch(void) {
    asm volatile(
            "ldr r1, %[rvbar]     \n"        // Load the value of rvbar0 into register r1
            "ldr r0, %[start]     \n"        // Load the value of start_addr into register r0
            "str r0, [r1]         \n"        // Store the value in register r0 to the address pointed by r1, which means storing start_addr to MMIO mapped RVBAR[0] register
            "dsb sy               \n"        // Data Synchronization Barrier to ensure memory access completion
            "isb sy               \n"        // Instruction Synchronization Barrier to ensure correct instruction execution order and prevent instruction reordering
            "mrc 15, 0, r0, cr12, cr0, 2  \n"// Read the value of Register Maintenance Register (RMR) from co-processor CP15 into register r0
            "orr r0, r0, #3       \n"        // Perform a bitwise OR operation between the value in register r0 and 0x3, setting the lowest two bits to 1, requesting a reset in AArch64 mode
            "mcr 15, 0, r0, cr12, cr0, 2  \n"// Write the value in register r0 to the RMR register in co-processor CP15, requesting a reset
            "isb sy               \n"        // Use the instruction synchronization barrier again
            "1: wfi               \n"        // Execute the Wait For Interrupt (WFI) instruction, waiting for an external event to wake up the system
            "b 1b                 \n"        // Jump back to the WFI instruction to continue waiting
            :
            : [rvbar] "m"(rvbar0), [start] "m"(start_addr)
            : "r0", "r1");
}