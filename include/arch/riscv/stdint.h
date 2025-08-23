/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file riscv64_stdint.h
 * @brief Standard integer type definitions for RISC-V 64-bit architecture.
 *
 * This header file defines fixed-width integer types using the 
 * typedefs from <types.h> to ensure consistent integer size 
 * across different platforms and architectures.
 */

#ifndef __RISCV_STDINT_H__
#define __RISCV_STDINT_H__

#include <types.h>

/** 
 * @brief Signed 8-bit integer type.
 */
typedef s8_t int8_t;

/** 
 * @brief Unsigned 8-bit integer type.
 */
typedef u8_t uint8_t;

/** 
 * @brief Signed 16-bit integer type.
 */
typedef s16_t int16_t;

/** 
 * @brief Unsigned 16-bit integer type.
 */
typedef u16_t uint16_t;

/** 
 * @brief Signed 32-bit integer type.
 */
typedef s32_t int32_t;

/** 
 * @brief Unsigned 32-bit integer type.
 */
typedef u32_t uint32_t;

/** 
 * @brief Signed 64-bit integer type.
 */
typedef s64_t int64_t;

/** 
 * @brief Unsigned 64-bit integer type.
 */
typedef u64_t uint64_t;

#endif// __RISCV_STDINT_H__
