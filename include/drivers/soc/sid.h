/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_SOC_SID_H__
#define __DRIVERS_SOC_SID_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <stdint.h>

#define SUNXI_SID_COMPATIBLE "allwinner,sunxi-sid"

typedef struct sunxi_sid {
	int dt_node;
	uintptr_t base;
	size_t size;
	uintptr_t efuse_hv_switch;
} sunxi_sid_t;

/**
 * @brief Read a 32-bit value from the SID SRAM mirror.
 *
 * @param sid SID controller instance.
 * @param offset Byte offset within the SRAM mirror.
 * @return The value read, or zero when the instance or offset is invalid.
 */
uint32_t sunxi_sid_read_sram(const sunxi_sid_t *sid, uint32_t offset);

/**
 * @brief Read a value from the eFuse memory.
 * 
 * This function reads a 32-bit value from the specified eFuse offset.
 * 
 * @param sid SID controller instance.
 * @param offset The offset in the eFuse memory from which to read the value.
 * @return The 32-bit value read from the specified eFuse offset.
 */
uint32_t sunxi_efuse_read(const sunxi_sid_t *sid, uint32_t offset);

/**
 * @brief Write a value to the eFuse memory.
 * 
 * This function writes a 32-bit value to the specified eFuse offset.
 * 
 * @param sid SID controller instance.
 * @param offset The offset in the eFuse memory to which the value will be written.
 * @param value The 32-bit value to be written to the eFuse memory.
 */
int sunxi_efuse_write(const sunxi_sid_t *sid, uint32_t offset,
		      uint32_t value);

/**
 * @brief Dump the contents of the eFuse memory.
 * 
 * This function outputs the current contents of the eFuse memory for 
 * diagnostic purposes. The format of the output is implementation-dependent.
 */
void sunxi_efuse_dump(const sunxi_sid_t *sid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DRIVERS_SOC_SID_H__ */
