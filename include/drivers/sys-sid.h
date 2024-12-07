/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _SYS_SID_H_
#define _SYS_SID_H_

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

#include <reg-ncat.h>

/**
 * @brief Read a value from the eFuse memory.
 * 
 * This function reads a 32-bit value from the specified eFuse offset.
 * 
 * @param offset The offset in the eFuse memory from which to read the value.
 * @return The 32-bit value read from the specified eFuse offset.
 */
uint32_t syter_efuse_read(uint32_t offset);

/**
 * @brief Write a value to the eFuse memory.
 * 
 * This function writes a 32-bit value to the specified eFuse offset.
 * 
 * @param offset The offset in the eFuse memory to which the value will be written.
 * @param value The 32-bit value to be written to the eFuse memory.
 */
void syter_efuse_write(uint32_t offset, uint32_t value);

/**
 * @brief Dump the contents of the eFuse memory.
 * 
 * This function outputs the current contents of the eFuse memory for 
 * diagnostic purposes. The format of the output is implementation-dependent.
 */
void syter_efuse_dump(void);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_SID_H_