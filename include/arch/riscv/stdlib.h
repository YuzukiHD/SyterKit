/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STDLIB_H__
#define __STDLIB_H__

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
int backtrace(char *PC, long *SP, char *LR);

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
int dump_stack(void);

#endif// __STDLIB_H__