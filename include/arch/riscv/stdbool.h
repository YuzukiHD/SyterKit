/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STDBOOL_H__
#define __STDBOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @file stdbool.h
 * @brief Standard boolean type definitions for C.
 *
 * This header file defines a standard boolean type for C, as well as the
 * boolean values `true` and `false`. It is intended to be compatible with C++
 * and to provide a clear definition of boolean logic.
 */

/**
 * @enum bool
 * @brief Boolean type enumeration.
 *
 * This enumeration defines the boolean values used in this implementation:
 * - `false` is defined as 0.
 * - `true` is defined as 1.
 */
enum {
	false = 0, /**< Boolean false value. */
	true = 1   /**< Boolean true value. */
};

/**
 * @typedef bool
 * @brief Boolean type definition.
 *
 * Defines the boolean type as an 8-bit signed integer (int8_t).
 * This allows for the representation of boolean values while ensuring
 * compatibility across different platforms.
 */
typedef int8_t bool;

#ifdef __cplusplus
}
#endif

#endif// __STDBOOL_H__
