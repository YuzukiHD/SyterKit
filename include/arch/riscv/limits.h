/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RISCV_LIMITS_H__
#define __RISCV_LIMITS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file riscv64_limits.h
 * @brief RISC-V 64-bit limits definitions.
 *
 * This header file defines the minimum and maximum values for
 * various data types according to the RISC-V 64-bit architecture.
 * These definitions are useful for ensuring proper handling of 
 * integer types and their ranges.
 */

/** Number of bits in a 'char' */
#define CHAR_BIT (8)

/** Minimum and maximum values a 'signed char' can hold */
#define SCHAR_MIN (-128) /**< Minimum value for signed char */
#define SCHAR_MAX (127)	 /**< Maximum value for signed char */

/** Minimum and maximum values a 'char' can hold */
#define CHAR_MIN SCHAR_MIN /**< Minimum value for char */
#define CHAR_MAX SCHAR_MAX /**< Maximum value for char */

/** Maximum value an 'unsigned char' can hold (Minimum is 0) */
#define UCHAR_MAX (255) /**< Maximum value for unsigned char */

/** Minimum and maximum values a 'signed short int' can hold */
#define SHRT_MIN (-1 - 0x7fff) /**< Minimum value for signed short int */
#define SHRT_MAX (0x7fff)	   /**< Maximum value for signed short int */

/** Maximum value an 'unsigned short int' can hold (Minimum is 0) */
#define USHRT_MAX (0xffff) /**< Maximum value for unsigned short int */

/** Minimum and maximum values a 'signed int' can hold */
#define INT_MIN (-1 - 0x7fffffff) /**< Minimum value for signed int */
#define INT_MAX (0x7fffffff)	  /**< Maximum value for signed int */
#define INT32_MAX (0x7fffffff)	  /**< Maximum value for 32-bit signed int */

/** Maximum value an 'unsigned int' can hold (Minimum is 0) */
#define UINT_MAX (0xffffffffU)	 /**< Maximum value for unsigned int */
#define UINT32_MAX (0xffffffffU) /**< Maximum value for 32-bit unsigned int */

/** Minimum and maximum values a 'signed long int' can hold */
#define LONG_MIN (-LONG_MAX - 1)	   /**< Minimum value for signed long int */
#define LONG_MAX (0x7fffffffffffffffL) /**< Maximum value for signed long int */

/** Maximum value an 'unsigned long int' can hold (Minimum is 0) */
#define ULONG_MAX (2UL * LONG_MAX + 1) /**< Maximum value for unsigned long int */

/** Minimum and maximum values a 'signed long long int' can hold */
#define LLONG_MIN (-LLONG_MAX - 1)		 /**< Minimum value for signed long long int */
#define LLONG_MAX (0x7fffffffffffffffLL) /**< Maximum value for signed long long int */

/** Maximum value an 'unsigned long long int' can hold (Minimum is 0) */
#define ULLONG_MAX (2ULL * LLONG_MAX + 1) /**< Maximum value for unsigned long long int */

/** Minimum and maximum values a 'max int' can hold */
#define INTMAX_MIN LLONG_MIN /**< Minimum value for max int */
#define INTMAX_MAX LLONG_MAX /**< Maximum value for max int */

/** Maximum value an 'max uint' can hold (Minimum is 0) */
#define UINTMAX_MAX ULLONG_MAX /**< Maximum value for max uint */

#ifdef __cplusplus
}
#endif

#endif /* __RISCV_LIMITS_H__ */
