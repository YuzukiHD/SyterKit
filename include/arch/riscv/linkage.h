/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RISCV_LINKAGE_H__
#define __RISCV_LINKAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file riscv64_linkage.h
 * @brief RISC-V 64-bit linkage definitions.
 *
 * This header file provides macro definitions for loading and storing
 * registers based on the RISC-V architecture's register size (32-bit
 * or 64-bit). The appropriate operations and sizes are chosen at 
 * compile time based on the defined architecture.
 */

#if __riscv_xlen == 64
/** Macro for loading a 64-bit value from memory. */
#define LREG ld

/** Macro for storing a 64-bit value to memory. */
#define SREG sd

/** Size of a register in bytes (64-bit). */
#define REGSZ 8

/** Directive for defining a 64-bit data type in assembly. */
#define RVPTR .dword
#elif __riscv_xlen == 32
/** Macro for loading a 32-bit value from memory. */
#define LREG lw

/** Macro for storing a 32-bit value to memory. */
#define SREG sw

/** Size of a register in bytes (32-bit). */
#define REGSZ 4

/** Directive for defining a 32-bit data type in assembly. */
#define RVPTR .word
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RISCV_LINKAGE_H__ */
