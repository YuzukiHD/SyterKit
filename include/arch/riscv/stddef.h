/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STDDEF_H__
#define __STDDEF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file stddef.h
 * @brief Standard definitions for C and C++.
 *
 * This file contains several macros and definitions used in both C and C++ code.
 * It defines types, utility macros, and compiler-specific optimizations for
 * portability across different compilers.
 */

/**
 * @def NULL
 * @brief Null pointer constant.
 *
 * Defines the value of NULL. In C++, NULL is defined as 0, while in C, it is defined as
 * a void pointer of value 0. The `NULL` macro can be used to represent null pointers
 * in both languages.
 */
#if defined(__cplusplus)
#define NULL (0)
#else
#define NULL ((void *) 0)
#endif

/**
 * @def offsetof(type, member)
 * @brief Get the offset of a member within a structure.
 *
 * Returns the byte offset of a given member within a structure. This is useful when
 * working with low-level memory manipulation or in custom memory allocation schemes.
 *
 * @param type The type of the structure.
 * @param member The member of the structure whose offset is to be calculated.
 * @return The offset of the member in bytes.
 */
#if (defined(__GNUC__) && (__GNUC__ >= 4))
#define offsetof(type, member) __builtin_offsetof(type, member)
#else
#define offsetof(type, field) ((size_t) (&((type *) 0)->field))
#endif

/**
 * @def container_of(ptr, type, member)
 * @brief Get a pointer to the container structure from a pointer to a member.
 *
 * This macro computes the address of the container structure from a pointer to a
 * member within the structure. This is useful for traversing linked data structures
 * like lists or trees.
 *
 * @param ptr Pointer to the member.
 * @param type Type of the structure containing the member.
 * @param member Name of the member in the structure.
 * @return Pointer to the container structure.
 */
#define container_of(ptr, type, member)                      \
	({                                                       \
		const typeof(((type *) 0)->member) *__mptr = (ptr);  \
		(type *) ((char *) __mptr - offsetof(type, member)); \
	})

/**
 * @def likely(expr)
 * @brief Likely expression hint for branch prediction.
 *
 * Provides a hint to the compiler that the given expression is likely to be true.
 * This helps the compiler optimize the code, improving performance in certain cases.
 *
 * @param expr Expression that is likely true.
 * @return The expression itself.
 */
#if (defined(__GNUC__) && (__GNUC__ >= 3))
#define likely(expr) (__builtin_expect(!!(expr), 1))
#define unlikely(expr) (__builtin_expect(!!(expr), 0))
#else
#define likely(expr) (!!(expr))
#define unlikely(expr) (!!(expr))
#endif

/**
 * @def min(a, b)
 * @brief Minimum of two values.
 *
 * Returns the smaller of two values, `a` and `b`.
 *
 * @param a First value.
 * @param b Second value.
 * @return The smaller value.
 */
#define min(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @def max(a, b)
 * @brief Maximum of two values.
 *
 * Returns the larger of two values, `a` and `b`.
 *
 * @param a First value.
 * @param b Second value.
 * @return The larger value.
 */
#define max(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @def clamp(v, a, b)
 * @brief Clamp a value between a minimum and maximum.
 *
 * This macro ensures that the value `v` is between `a` and `b`. If `v` is smaller than
 * `a`, it returns `a`, and if `v` is greater than `b`, it returns `b`.
 *
 * @param v Value to be clamped.
 * @param a Minimum bound.
 * @param b Maximum bound.
 * @return The clamped value.
 */
#define clamp(v, a, b) min(max(a, v), b)

/**
 * @def ifloor(x)
 * @brief Integer floor of a floating-point value.
 *
 * Computes the integer floor of a floating-point value `x`. If `x` is positive, it truncates
 * towards zero; otherwise, it rounds down to the nearest integer.
 *
 * @param x The floating-point value.
 * @return The integer floor of `x`.
 */
#define ifloor(x) ((x) > 0 ? (int) (x) : (int) ((x) -0.9999999999))

/**
 * @def iround(x)
 * @brief Integer rounding of a floating-point value.
 *
 * Rounds a floating-point value `x` to the nearest integer. Rounds up if `x` is positive
 * and 0.5 or more, and rounds down otherwise.
 *
 * @param x The floating-point value.
 * @return The nearest integer to `x`.
 */
#define iround(x) ((x) > 0 ? (int) ((x) + 0.5) : (int) ((x) -0.5))

/**
 * @def iceil(x)
 * @brief Integer ceiling of a floating-point value.
 *
 * Computes the ceiling of a floating-point value `x`, rounding up to the nearest integer.
 *
 * @param x The floating-point value.
 * @return The smallest integer greater than or equal to `x`.
 */
#define iceil(x) ((x) > 0 ? (int) ((x) + 0.9999999999) : (int) (x))

/**
 * @def idiv255(x)
 * @brief Divide an integer by 255 efficiently.
 *
 * Performs division of an integer by 255, with an efficient algorithm using bit shifts.
 * This is useful for applications where division by 255 is common, such as image processing.
 *
 * @param x The integer value.
 * @return The result of dividing `x` by 255.
 */
#define idiv255(x) ((((int) (x) + 1) * 257) >> 16)

#ifdef __cplusplus
}
#endif

#endif /* __STDDEF_H__ */
