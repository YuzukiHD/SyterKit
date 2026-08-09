/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file
 * @brief Allwinner Platform True Random Number Generator (TRNG) Driver
 *
 * This file implements the True Random Number Generator driver for Allwinner platforms.
 * The TRNG driver provides functionality for generating cryptographically secure
 * random numbers from hardware entropy sources. This is essential for security-related
 * operations such as key generation, encryption initialization vectors, and secure
 * random values for various cryptographic protocols.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include <drivers/clk.h>
#include <drivers/rng.h>
