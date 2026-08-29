/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "dram-sun55iw6: " fmt

/**
 * @file dram-sun55iw6.c
 * @brief DRAM controller driver for the Allwinner sun55iw6 SoC.
 *
 * Registers the DRAM driver with the DT2C driver framework. DRAM
 * initialization is performed by the blob loaded at runtime.
 */

#include <barrier.h>
#include <io.h>
#include <mmu.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <dt2c/driver.h>
#include <drivers/dram/dram.h>
#include <drivers/rtc/rtc.h>

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
