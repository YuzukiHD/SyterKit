/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_RPROC_H__
#define __REG_RPROC_H__

#include <reg-ncat.h>

#define SUNXI_A27L_WFI_MODE_REG (SUNXI_A27L2_CFG_BASE + 0x4)
#define SUNXI_A27L_START_ADD_REG (SUNXI_A27L2_CFG_BASE + 0x204)

#define SUNXI_WAKUP_CTRL_REG (SUNXI_PMU_AON_BASE + 0x64)
#define SUNXI_A27L_WAKUP_EN (0x1 << 8)
#define SUNXI_LP_STATUS_REG (SUNXI_PMU_AON_BASE + 0x68)
#define SUNXI_A27L_STATUS (0x1 << 12)

#define SUNXI_E907_LPMD_MODE_MASK "0xC"
#define SUNXI_E907_LPMD_MODE_LILGHTSLP "0x4"

/**
 * @brief Boot the A27L2 processor.
 * 
 * This function configures clocks, reset controls, and cache settings to boot the A27L processor.
 * It performs a series of hardware register operations to ensure the processor is in the correct state 
 * during boot-up.
 * 
 * @param addr The address to start the A27L processor. This is typically the entry point of a program or firmware.
 * 
 * @note 
 * - This function disables interrupts on entry and performs necessary clock and reset settings before 
 *   booting the A27L2 processor.
 * - Ensure that relevant registers and addresses are properly initialized before calling this function.
 * - The function uses assembly instructions to directly manipulate hardware registers, thus requiring 
 *   the compiler to understand the embedded assembly code.
 * 
 * @warning 
 * - Make sure the relevant hardware state is ready before invoking this function to avoid undefined behavior.
 */
void sunxi_ansc_boot(uint32_t addr);

#endif// __REG_RPROC_H__